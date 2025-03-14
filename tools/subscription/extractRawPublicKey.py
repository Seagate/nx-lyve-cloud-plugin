import base64
import sys
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
import argparse

def extract_key(public_key_path):
    try:
        with open(public_key_path, 'rb') as key_file:
            public_key = serialization.load_ssh_public_key(
                key_file.read(), backend=default_backend()
            )

        raw_public_key_bytes = public_key.public_bytes(encoding=serialization.Encoding.Raw, format=serialization.PublicFormat.Raw)
        return base64.b64encode(raw_public_key_bytes).decode('utf-8')

    except Exception as e:
        print(f'Error loading public key. Here\'s why: {type(e)}: {e}', file=sys.stderr)
        return None

def main():
    # CLI parser
    parser = argparse.ArgumentParser()
    parser.add_argument('public_key_path', help='path to the public key, used to verify the subscription')
    args = parser.parse_args()
    
    # extract key data
    raw_public_key_bytes_base64 = extract_key(args.public_key_path)

    # print output
    if raw_public_key_bytes_base64:
        print(f'BEGIN RAW PUBLIC KEY BYTES (BASE64)\n{raw_public_key_bytes_base64}\nEND RAW PUBLIC KEY BYTES (BASE64)')
        return 0
    else:
        print('Failed to extract public key.', file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main())