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

const encrypt = (data, publicKey) =>
    crypto.publicEncrypt(
        {
            key: publicKey,
            padding: crypto.constants.RSA_PKCS1_OAEP_PADDING,
            oaepHash: 'sha256',
        },
        Buffer.from(data),
    );

const decrypt = (data, privateKey) =>
    crypto
        .privateDecrypt(
            {
                key: privateKey,
                padding: crypto.constants.RSA_PKCS1_OAEP_PADDING,
                oaepHash: 'sha256',
            },
            data,
        )
        .toString();

(function main() {
    const data = 'supersecretpassword';
    const keyPair = generateKeyPair();
    const encrypted = encrypt(data, keyPair.public);
    const decrypted = decrypt(encrypted, keyPair.private);

    console.dir({
        isEqual: decrypted === data,
        data,
        encrypted: encrypted.toString(),
        decrypted,
    });
})();