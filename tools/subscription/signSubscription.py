import json
import base64
import sys
import getpass
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
import argparse

def sign_data(subscription_data, private_key_path, passphrase):
    # Prompt the user for the passphrase securely
    try:
        with open(private_key_path, 'rb') as keyFile:
            private_key = serialization.load_ssh_private_key(
                keyFile.read(), password=passphrase, backend=default_backend()
            )
        
        signature = private_key.sign(subscription_data)
        return base64.b64encode(signature).decode()

    except Exception as e:
        print(f'Error signing: {type(e)}: {e}', file=sys.stderr)
        return None

def main():
    # CLI parser
    parser = argparse.ArgumentParser(
        prog='signSubscription',
        description='Sign JSON data from stdin using the given private key')
    parser.add_argument('private_key_path', help='path to private key file')
    args = parser.parse_args()

    # gather all input first (read until EOF)
    subscriptionsString = sys.stdin.read()
    # initialize
    passphrase = getpass.getpass(prompt='Enter private key passphrase (empty for no passphrase): ').encode()
    decoder = json.JSONDecoder()
    startPosition = 0
    objectsParsed = 0
    while True:
        try:
            # skip whitespace
            while startPosition + 1 < len(subscriptionsString) and subscriptionsString[startPosition].isspace():
                startPosition += 1
            # parse the next subscription JSON object
            subscription_info, startPosition = decoder.raw_decode(subscriptionsString, startPosition)
            objectsParsed += 1
            # serialize it in a standard way
            subscription_data = json.dumps(subscription_info, sort_keys=True).encode()
            
            # get a signature
            signature = sign_data(subscription_data, args.private_key_path, passphrase)
            if not signature:
                print('Failed to sign subscription', file=sys.stderr)
                return 1
            
            # combine subscription info and signature
            subscription_data_base64 = base64.b64encode(subscription_data).decode()
            combined_data = {
                'subscriptionInfo': subscription_data_base64,
                'signature': signature,
            }
            subscription_key = base64.b64encode(json.dumps(combined_data).encode()).decode()
            
            # print key (or report failure)
            if subscription_key:
                print(subscription_key)
            else:
                print('Failed to generate subscription key.', file=sys.stderr)
                return 1
            
        except json.decoder.JSONDecodeError as e:
            if objectsParsed == 0:
                print(f'No valid JSON input data found: {type(e)}: {e}', file=sys.stderr)
                return 1
            return 0
        except Exception as e:
            print(f"Failed to generate subscription key. Here\'s why: {type(e)}: {e}", file=sys.stderr)
            return 1

if __name__ == '__main__':
    # call main function
    sys.exit(main())