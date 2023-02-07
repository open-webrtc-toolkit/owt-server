// Generate request header to OWT service
'use strict';

const path = require('path');
const crypto = require('crypto');

console.log(path.basename(__filename));
let args = process.argv;
while (args[0].indexOf(path.basename(__filename)) === -1) {
    args.shift();
}

if (args.length < 3) {
    console.error('Usage: genHeader.js SERVICE_ID SERVICE_KEY');
    process.exit(1);
}

const serviceid = args[1];
const servicekey = args[2];

function calculateSignature(toSign, key) {
  const hash = crypto.createHmac("sha256", key).update(toSign);
  const hex = hash.digest('hex');
  return Buffer.from(hex).toString('base64');
}

let header = 'MAuth realm=http://marte3.dit.upm.es,mauth_signature_method=HMAC_SHA256';
const timestamp = new Date().getTime();
const cnounce = crypto.randomInt(99999);
const toSign = timestamp + ',' + cnounce;
header += ',mauth_serviceid=';
header += serviceid;
header += ',mauth_cnonce=';
header += cnounce;
header += ',mauth_timestamp=';
header += timestamp;
header += ',mauth_signature=';
header += calculateSignature(toSign, servicekey);

console.log(header);
      
