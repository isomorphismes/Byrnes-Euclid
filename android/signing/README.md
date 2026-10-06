# Stable public development signer

`development.p12` is the existing public organization test key, copied exactly
from `isomorphismes/wegert@dfa3751f0282cdf6a2083c97063b0e8e996d0d92`,
`_/build/app/wegert-debug.keystore`. This is sideload-only test material.
Alias, store password and key password: `wegert-debug`. No key generation.

Keystore SHA-256:
`d83f7a36a205e49b4c755f37d27a0ded02779acc0da82a22e19d458dc6e5a89a`.
Certificate SHA-256:
`de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9`.

Build preflight verifies the key file and exports/checks its certificate.
Final APK verification checks the signer independently. Missing or mismatched
material fails the build. Keep this identity for replacement installs.
