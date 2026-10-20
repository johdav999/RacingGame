import ctypes
import getpass
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import warnings

SECRET = '.secrets/tripo.dpapi'

def repo_root(value):
    root = Path(value).resolve(strict=True)
    result = subprocess.run(['git', '-C', str(root), 'rev-parse', '--show-toplevel'],
                            capture_output=True, text=True)
    if result.returncode or Path(result.stdout.strip()).resolve() != root:
        raise ValueError('--repo must be a Git repository root')
    return root


def within(root, relative):
    path = (root / relative).resolve()
    if not path.is_relative_to(root) or path == root:
        raise ValueError('Path must stay inside the repository')
    return path


def secret_path(root):
    path = within(root, SECRET)
    tracked = subprocess.run(['git', '-C', str(root), 'ls-files', '--', '.secrets'],
                             capture_output=True, text=True, check=True)
    ignored = subprocess.run(['git', '-C', str(root), 'check-ignore', '-q', '--', SECRET],
                             capture_output=True)
    if tracked.stdout.strip() or ignored.returncode != 0:
        raise ValueError('Add /.secrets/ to .gitignore and ensure no .secrets files are tracked')
    return path


def crypt(data, decrypt=False):
    if os.name != 'nt':
        raise ValueError('Encrypted file storage requires Windows; use TRIPO_API_KEY here')
    from ctypes import wintypes

    class Blob(ctypes.Structure):
        _fields_ = [('size', wintypes.DWORD), ('data', ctypes.POINTER(ctypes.c_ubyte))]

    buffer = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
    source = Blob(len(data), buffer)
    target = Blob()
    library = ctypes.WinDLL('crypt32', use_last_error=True)
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    function = library.CryptUnprotectData if decrypt else library.CryptProtectData
    function.argtypes = [ctypes.POINTER(Blob), ctypes.c_void_p, ctypes.POINTER(Blob),
                         ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(Blob)]
    function.restype = wintypes.BOOL
    kernel.LocalFree.argtypes = [ctypes.c_void_p]
    kernel.LocalFree.restype = ctypes.c_void_p
    if not function(ctypes.byref(source), None, None, None, None, 1, ctypes.byref(target)):
        raise ValueError('Windows credential protection failed; configure under this Windows login')
    try:
        return ctypes.string_at(target.data, target.size)
    finally:
        ctypes.memset(target.data, 0, target.size)
        kernel.LocalFree(target.data)
        ctypes.memset(buffer, 0, len(data))


def get_key(root):
    key = os.environ.get('TRIPO_API_KEY', '').strip()
    if key:
        return key
    path = secret_path(root)
    if not path.is_file():
        raise ValueError('No key configured. Run configure in your own terminal first')
    return crypt(path.read_bytes(), decrypt=True).decode('utf-8')


def configure(root):
    path = secret_path(root)
    if os.name != 'nt':
        raise ValueError('Use TRIPO_API_KEY on non-Windows hosts')
    if not sys.stdin.isatty():
        raise ValueError('Run configure in your own interactive terminal; never pass a key in chat')
    with warnings.catch_warnings():
        warnings.simplefilter('error', getpass.GetPassWarning)
        key = getpass.getpass('Tripo API key (hidden): ').strip()
    if not key or any(c.isspace() for c in key):
        raise ValueError('Key must be nonempty and contain no whitespace')
    encrypted = crypt(key.encode('utf-8'))
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as stream:
            temp = Path(stream.name)
            stream.write(encrypted)
        os.replace(temp, path)
    finally:
        if temp is not None:
            temp.unlink(missing_ok=True)
    print('Encrypted key stored; Git ignore verified. No API call made.')


