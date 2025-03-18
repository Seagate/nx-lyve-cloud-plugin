import json
import sys
import argparse

def main():
    # CLI parser
    parser = argparse.ArgumentParser(
        prog='setSubscriptionCapacity',
        description='Add a subscription capacity to JSON subscription info from stdin')
    parser.add_argument('capacity_gb', nargs='?', type=int, default=2000, help='Desired storage subscription capacity (default 2000 GB)')
    args = parser.parse_args()

    # parse subscription data from stdin
    try:
        subscription_info = json.loads(sys.stdin.read())
    except Exception as e:
        print(f'Error parsing subscription data: {type(e)}: {e}', file=sys.stderr)
        return 1
    
    # add the capacity
    subscription_info['bucket-capacity-gb'] = args.capacity_gb

    # serialize as JSON and send to stdout
    print(json.dumps(subscription_info))
    return 0

if __name__ == '__main__':
    # call main function
    sys.exit(main())