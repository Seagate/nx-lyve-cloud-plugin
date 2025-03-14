import json
import base64
import sys
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
from cryptography.exceptions import InvalidSignature
import argparse

def verify_signature(subscription_key_data_base64, public_key_path):
    try:
        # read public key
        with open(public_key_path, 'rb') as public_key_file:
            public_key = serialization.load_ssh_public_key(
                public_key_file.read(), backend=default_backend()
            )
        
        # decode subscription key
        subscription_key = json.loads(base64.b64decode(subscription_key_data_base64))
        
        # split subscription key into its data and its signature
        subscription_data = base64.b64decode(subscription_key['subscriptionInfo'])
        subscription_signature = base64.b64decode(subscription_key['signature'])
        
        # verify signature using public key
        public_key.verify(subscription_signature, subscription_data)
        return True
    
    except InvalidSignature:
        print('Signature does not match data', file=sys.stderr)
    except Exception as e:
        print(f'Failed to verify signature. Here\'s why: {type(e)}: {e}', file=sys.stderr)
    return False

def main():
    # CLI parser
    parser = argparse.ArgumentParser(
        prog='verifySignature',
        description='Verify subscription key from stdin using the given public key')
    parser.add_argument('public_key_path', help='path to public key file')
    args = parser.parse_args()
    
    # get subscription key from stdin
    subscription_key_data_base64 = sys.stdin.read().strip()
    # verify signature
    if verify_signature(subscription_key_data_base64, args.public_key_path):
        print('Signature is valid.')
        return 0
    else:
        print('Signature verification failed.', file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main())