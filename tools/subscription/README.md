# Debugging with Subscription Keys

## Motivation

The plugin only takes signed subscription keys, but the DW private key must not be shared, even for debugging.
The solution is to change the source to use a debug signature key.

## Instructions

On a bash terminal:

1. Generate your own key pair:  
`ssh-keygen -t ed25519 -f debugKey`
2. Put your public key into the plugin:
    1. Extract the raw key data in base64 from the public key:  
    `python extractRawPublicKey.py debugKey.pub`  
    Note: if that Python script fails for dependencies, you may need to install cryptography using pip.
    2. Put the output (*not* including the surrounding text) into the publicKeyRawBase64 variable in engine.cpp in the plugin source.
    3. Build and install the plugin.
3. Create your subscription key:
    1. In tools/subscription, use exampleSubscriptionInfo.json as a template for your S3 credentials.
    2. Sign your S3 credentials with your private key using the Python script there:  
    `cat exampleSubscriptionInfo.json | python signSubscription.py debugKey`  
    The output is your subscription key, which you can provide to a customer, to paste into their DW Cumulus plugin settings.

## Subscription Key Tools

- Verify a Subscription Key Signature  
Try this with a signed subscription key:  
`cat exampleSubscriptionKey | python verifySignature.py debugKey.pub`
- Extract Subscription Info  
Since subscription keys are signed, but not encrypted, we can easily extract the information from any subscription key:  
`cat exampleSubscriptionKey | python extractSubscription.py`
- Set Subscription Capacity  
This tool takes in a subscription JSON, and (over)writes the `bucket-capacity-gb` field:  
`cat exampleProvisioningOutput.json | python setSubscriptionCapacity.py 4000`  
Always use this tool to set the subscription capacity before signing the subsciption.  
You can also use this in combination with extractSubscription.py to rewrite an existing subscription key with a different capacity:  
`cat exampleSubscriptionKey | python extractSubscription.py | python setSubscriptionCapacity.py 8000 | python signSubscription.py debugKey`