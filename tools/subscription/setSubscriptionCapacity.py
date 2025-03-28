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

    # gather all input first (read until EOF)
    subscriptionsString = sys.stdin.read()
    # initialize
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
            # add the capacity
            subscription_info['bucket-capacity-gb'] = args.capacity_gb
            # serialize as JSON and send to stdout
            print(json.dumps(subscription_info))
        except Exception as e:
            if objectsParsed == 0:
                print(f'No valid JSON input data found. {type(e)}: {e}', file=sys.stderr)
                return 1
            return 0

if __name__ == '__main__':
    # call main function
    sys.exit(main())