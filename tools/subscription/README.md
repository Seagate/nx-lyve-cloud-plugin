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
    2. Put the output into the publicKeyRawBase64 variable in engine.cpp in the plugin source.
    3. Build and install the plugin.
3. Create your subscription key:
    1. In tools/subscription, use exampleSubscriptionInfo.json as a template for your S3 credentials.
    2. Sign your S3 credentials with your private key using the Python script there:  
    `cat exampleSubscriptionInfo.json | python signSubscription.py debugKey`  
    The output is your subscription key, which you can paste into the plugin settings.