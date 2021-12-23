'use strict';

const crypto = require('crypto');

const generateKeyPair = (length = 2048) => {
  const {publicKey, privateKey} = crypto.generateKeyPairSync('rsa', {
    modulusLength: length,
  });
  return {
    public: publicKey,
    private: privateKey,
  };
};

const signMessage = (message, privateKey) => {
  const signature = crypto.sign('sha256', Buffer.from(message), {
    key: privateKey,
    padding: crypto.constants.RSA_PKCS1_PSS_PADDING,
  });
  return signature;
};

const verify = (message, signature, publicKey) => {
  const isCorrect = crypto.verify(
    'sha256',
    Buffer.from(message),
    {
      key: publicKey,
      padding: crypto.constants.RSA_PKCS1_PSS_PADDING,
    },
    signature,
  );
  return isCorrect;
};

(function main() {
  try {
    const message = 'supersecretpassword';
    const keys = generateKeyPair();
    const signature = signMessage(message, keys.private);
    const invalidSignature = Buffer.from(new Array(10).fill(0).map(() => 1));
    console.dir({
      case: 'validSig',
      message,
      signature: signature.toString(),
      isCorrect: verify(message, signature, keys.public),
    });
    console.dir({
      case: 'invalidSig',
      message,
      signature: invalidSignature.toString(),
      isCorrect: verify(message, invalidSignature, keys.public),
    });
  } catch (err) {
    console.log(err);
  }
})();