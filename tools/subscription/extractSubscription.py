import json
import base64
import sys
import argparse

def get_subscription(subscription_key_data_base64):
    try:
        # decode subscription key
        subscription_key = json.loads(base64.b64decode(subscription_key_data_base64))
        # split subscription key into its data and its signature
        return base64.b64decode(subscription_key['subscriptionInfo']).decode()
    except Exception as e:
        print(f'Failed to extract subscripiton info. Here\'s why: {type(e)}: {e}', file=sys.stderr)
        return None

def main():
    # CLI parser
    parser = argparse.ArgumentParser(
        prog='extractSubscription',
        description='Extract subscription info from the given subscription key')
    parser.parse_args()
    
    # get subscription key from stdin
    subscription_key_data_base64 = sys.stdin.read().strip()
    # verify signature
    subscription_info = get_subscription(subscription_key_data_base64)
    if subscription_info:
        print(subscription_info)
        return 0
    else:
        return 1

if __name__ == '__main__':
    sys.exit(main())