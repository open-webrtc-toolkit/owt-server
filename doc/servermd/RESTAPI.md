WebRTC Conference Management RESTAPI
-------
# 1 Introduction {#RESTAPIsection1}
Intel WebRTC solution provides a set of REST APIs for conference management. Manager clients can be implemented by different programming languages.

# 2 Authentication and Authorization {#RESTAPIsection2}
Each request should be a HTTP request with "Authorization" field in the header. So the server can determine whether it is a valid client. The value of "Authorization" is something like:
~~~~~~~~~~~~~{.js}
MAuth　realm=http://webrtc.intel.com,
mauth_signature_method=HMAC_SHA256,
mauth_username=test,
mauth_role=role,
mauth_serviceid=53c74879209ee7f96e5cbc9c,
mauth_cnonce=87428,
mauth_timestamp=1406079112038,
mauth_signature=ZjA5NTJlMjE0ZTY4NzhmZWRjZDkxYjNmZjkyOTIxYzMyZjg3NDBjZA==
~~~~~~~~~~~~~

 - MAuth realm=http://webrtc.intel.com,mauth_signature_method=HMAC_SHA256 is a constant string which should not be changed.
 - mauth_username and mauth_role are optional.
 - mauth_serviceid is the ID of the service.
 - mauth_cnonce is a random value between 0 and 99999.
 - mauth_timestamp is the timestamp of the request.
 - mauth_signature is signature of this request. It uses HMAC_SHA256 to sign timestamp, cnonce, username(optional), role(optional) with service key. Then encrypted it with BASE64.

Example of encryption algorithm (python):
~~~~~~~~~~~~~{.py}
toSign = str(timestamp) + ',' + str(cnounce);
if (username != '' and role != ''):
  toSign += ',' + username + ',' + role;
signed = self.calculateSignature(toSign, key);
def calculateSignature(self, toSign, key):
  from hashlib import HMAC_SHA256
  import hmac
  import binascii
  hash = hmac.new(key, toSign, HMAC_SHA256);
  signed = binascii.b2a_base64(hash.hexdigest())[:-1];
  return signed;
~~~~~~~~~~~~~

toSign is the signature.

**Note:
If authentication failed, a response with code 401 would be returned.
~~~~~~~~~~~~~{.js}
{'WWW-Authenticate': 'MAuth realm="http://marte3.dit.upm.es"'}
~~~~~~~~~~~~~

# 3 APIs {#RESTAPIsection3}

## 3.1 Service Management APIs {#RESTAPIsection3_1}

### 3.1.1 Get Services {#RESTAPIsection3_1_1}

This API lists all the services that available.

**URL**

        http://<hostname>:3000/services

**HTTP Method**

        GET

**Response Example**

Status 200:
~~~~~~~~~~~~~{.js}
[{
  "_id" : "57bc05f1cea2902dd65a297c"
  "name" : "superService",
  "key" : "91f186c9cd5e345751fdff8174c1f133dc08084e5e4d4037866445f396e9d774818c1577be849aa6af1637ab94efcb96f8ae9a62a9682aee9d7b01e2f4cd0f536af65c1a4fb647dbb0cf59fe12e59452b4708faeec823fe749f980617131e113a4efbf3827c17a72db2ade03983bccf9387ef862329919188e7d8ab08f5afb4a1a2fe1f6531a36f74da3c2fd8be15c13f7b2e67aa3224f6b367fc9585dbb0cec2824b28e3816f2e9304178a2",
  "encrypted" : true,
  "rooms" : [ ],
},
{
  "_id" : "57c67027596f657b1ef2c4f8"
  "name" : "sampleService",
  "key" : "bad5f2d7ca3c04556afce4d557f5a329e63c6e3d2f364137d85e71fce88ee6288bd23d7c8bc08db3a36529dab9d9e383fedf9b36d33131cd825f2094d8cb0a2360c6693c6d8258f5e59561a85debce64aa5ab396a0ac179b0add826c7f34dd2bc8cbee0218bf5f50da75e408eb0291ae3735ed6231c100099230b6948e3dc70579079a80586e15e561acb1d1e3805c44c0a6980396527d6c3c5ea6645fb70fb1187d94c03e3fa1ee1c0942a2",
  "encrypted" : true,
  "rooms" : [{
      "name" : "testRoom",
      "mode" : "hybrid",
      "publishLimit" : 3,
      "userLimit" : -1,
      "mediaMixing" : {
        "video" : {
          "avCoordinated" : 0,
          "maxInput" : 16,
          "resolution" : "vga",
          "multistreaming" : 0,
          "bitrate" : 0,
          "bkColor" : "black",
          "crop" : 0,
          "layout" : {
            "base" : "fluid",
            "custom" : [ ]
          }
        },
        "audio" : null
      },
      "enableMixing" : 1,
      "sipInfo" : null,
    }],
}]
~~~~~~~~~~~~~

Status 401:
~~~~~~~~~~~~~{.js}
Service not authorized for this action
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Service not found
~~~~~~~~~~~~~

### 3.1.2 Create Service {#RESTAPIsection3_1_2}

This API creates a service.

**URL**

        http://<hostname>:3000/services/

**HTTP Method**

        POST

**Parameters**

 - **Required**: A JSON string.  Following attributes are allowed:
~~~~~~~~~~~~~{.js}
{
  "name" : "helloService",
  "key" : "xxxxxxxxxxxxxx"
}
~~~~~~~~~~~~~

**Note:
The key will be encrypted in the server.

**Response Example**

Status 200:
~~~~~~~~~~~~~{.js}
{
  "_id" : "57bc05f1cea2902dd65a297c"
  "name" : "helloService",
  "key" : "91f186c9cd5e345751fdff8174c1f133dc08084e5e4d4037866445f396e9d774818c1577be849aa6af1637ab94efcb96f8ae9a62a9682aee9d7b01e2f4cd0f536af65c1a4fb647dbb0cf59fe12e59452b4708faeec823fe749f980617131e113a4efbf3827c17a72db2ade03983bccf9387ef862329919188e7d8ab08f5afb4a1a2fe1f6531a36f74da3c2fd8be15c13f7b2e67aa3224f6b367fc9585dbb0cec2824b28e3816f2e9304178a2",
  "encrypted" : true,
  "rooms" : [],
}
~~~~~~~~~~~~~

Status 401:
~~~~~~~~~~~~~{.js}
Service not authorized for this action
~~~~~~~~~~~~~

Status 401:
~~~~~~~~~~~~~{.js}
Service name and key do not have string type
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Service not found
~~~~~~~~~~~~~

### 3.1.3 Get Service {#RESTAPIsection3_1_3}

This API returns a specific service.

**URL**

        http://<hostname>:3000/services/[service id]

**HTTP Method**

        GET

**Response Example**

Status 200:
~~~~~~~~~~~~~{.js}
{
  "_id" : "57bc05f1cea2902dd65a297c"
  "name" : "superService",
  "key" : "91f186c9cd5e345751fdff8174c1f133dc08084e5e4d4037866445f396e9d774818c1577be849aa6af1637ab94efcb96f8ae9a62a9682aee9d7b01e2f4cd0f536af65c1a4fb647dbb0cf59fe12e59452b4708faeec823fe749f980617131e113a4efbf3827c17a72db2ade03983bccf9387ef862329919188e7d8ab08f5afb4a1a2fe1f6531a36f74da3c2fd8be15c13f7b2e67aa3224f6b367fc9585dbb0cec2824b28e3816f2e9304178a2",
  "encrypted" : true,
  "rooms" : [ ],
}
~~~~~~~~~~~~~

Status 401:
~~~~~~~~~~~~~{.js}
Service not authorized for this action
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Service not found
~~~~~~~~~~~~~

### 3.1.4 Delete Service {#RESTAPIsection3_1_4}

This API deletes a specific service.

**URL**

        http://<hostname>:3000/services/[service id]

**HTTP Method**

        DELETE

**Response Example**

Status 200:
~~~~~~~~~~~~~{.js}
Service deleted
~~~~~~~~~~~~~

Status 401:
~~~~~~~~~~~~~{.js}
Service not authorized for this action
~~~~~~~~~~~~~

Status 401:
~~~~~~~~~~~~~{.js}
Super service not permitted to be deleted
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Service not found
~~~~~~~~~~~~~


## 3.2 Room Management APIs {#RESTAPIsection3_2}

### 3.2.1 Get Rooms {#RESTAPIsection3_2_1}

This API lists all the rooms that available in current service.

**URL**

        http://<hostname>:3000/rooms

**HTTP Method**

        GET

**Response Example**

Status 200:
~~~~~~~~~~~~~{.js}
[{
  "_id":"57bc06145be6d22e13a2b81e"
  "name":"myRoom",
  "mode":"hybrid",
  "publishLimit":-1,
  "userLimit":-1,
  "mediaMixing":{
    "video":{
      "avCoordinated":0,
      "maxInput":16,
      "resolution":"vga",
      "multistreaming":0,
      "bitrate":0,
      "bkColor":"black",
      "crop":0,
      "layout":{
        "base":"fluid",
        "custom":[]
      }
    },
    "audio":null
  },
  "enableMixing":1,
  "sipInfo":{
    "sipServer":"webrtc.sh.intel.com",
    "username":"webrtc2",
    "password":"123"
  }
},
{
  "_id":"57c67027596f657b1ef2c4f8"
  "name":"testRoom",
  "mode":"hybrid",
  "publishLimit":3,
  "userLimit":-1,
  "mediaMixing":{
    "video":{
      "avCoordinated":0,
      "maxInput":16,
      "resolution":"sif",
      "multistreaming":0,
      "bitrate":0,
      "bkColor":"black",
      "crop":0,
      "layout":{
        "base":"fluid",
        "custom":[]
      }
    },
    "audio":null
  },
  "enableMixing":1,
  "sipInfo":null
}]
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Service not found
~~~~~~~~~~~~~

### 3.2.2 Create Room {#RESTAPIsection3_2_2}

This API creates a room in current service.

**URL**

        http://<hostname>:3000/rooms/

**HTTP Method**

        POST

**Parameters**

 - **Option**: A JSON string.  Following attributes are allowed:
~~~~~~~~~~~~~{.js}
{
  "name": "apiRoom", // type string, required
  options: { // the attributes in options are all optional
    "mode": "hybrid", // type string, only support "hybrid" in current release
    "publishLimit": 16, // type number, -1 means unlimited
    "userLimit": 100, // type number, -1 means unlimited
    "enableMixing": 1, // type number: 0/1
    "mediaMixing": { // type object
      "video": {
        "resolution": "vga", // type string, valid values ["vga", "sif", "xga", "svga", "hd720p", "hd1080p", "uhd_4k", "r720x720"]
        "bitrate": 0, // type number
        "bkColor": {  // or just use "black" with type string
          "r": 255, // type number, 0 ~ 255
          "g": 255, // type number, 0 ~ 255
          "b": 255  // type number, 0 ~ 255
        },
        "maxInput": 16, // type number
        "avCoordinated": 0, // type number: 0/1
        "multistreaming": 0, // type number: 0/1
        "crop": 0, // type number: 0/1
        "layout": { // type object
          "base": "fluid", // type string, valid values ["fluid", "lecture", "void"]
          "custom": [] // type object::Array or null, the format is introduced in MCU server guide, if layout base is "void", then "custom" can not be null
        }
      },
      "audio": null // type object, currently only support null
    },
    "sipInfo":{ // optional
      "sipServer":"webrtc.sip.com", // type string
      "username":"imUser", // type string
      "password":"imPassword" // type string
    }
  }
}
~~~~~~~~~~~~~

If no parameters provided, default test room would be created.

**Response Example**

Status 200:
~~~~~~~~~~~~~{.js}
{
  "_id":"57c67027596f657b1ef2c4f8"
  "name": "apiRoom",
  "mode":"hybrid",
  "publishLimit":16,
  "userLimit":100,
    "enableMixing": 1,
    "mediaMixing": {
      "video": {
        "resolution": "vga",
        "bitrate": 0,
        "bkColor": {
          "r": 255,
          "g": 255,
          "b": 255
        },
        "maxInput": 16,
        "avCoordinated": 0,
        "multistreaming": 0,
        "crop": 0,
        "layout": {
          "base": "fluid",
          "custom": []
        }
      },
      "audio": null
    },
    "sipInfo":{
      "sipServer":"webrtc.sip.com",
      "username":"imUser",
      "password":"imPassword"
    }
}
~~~~~~~~~~~~~

Status 400:
~~~~~~~~~~~~~{.js}
Invalid room
~~~~~~~~~~~~~

Status 400:
~~~~~~~~~~~~~{.js}
Invalid room optional
~~~~~~~~~~~~~

Status 400:
~~~~~~~~~~~~~{.js}
Bad room configuration
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Service not found
~~~~~~~~~~~~~

### 3.2.3 Get Room {#RESTAPIsection3_2_3}

This API returns a specific room in current service.

**URL**

        http://<hostname>:3000/rooms/[room id]

**HTTP Method**

        GET

**Response Example**

Status 200:
~~~~~~~~~~~~~{.js}
{
  "_id":"57c67027596f657b1ef2c4f8"
  "name":"testRoom",
  "mode":"hybrid",
  "publishLimit":3,
  "userLimit":-1,
  "mediaMixing":{
    "video":{
      "avCoordinated":0,
      "maxInput":16,
      "resolution":"sif",
      "multistreaming":0,
      "bitrate":0,
      "bkColor":"black",
      "crop":0,
      "layout":{
        "base":"fluid",
        "custom":[]
      }
    },
    "audio":null
  },
  "enableMixing":1,
  "sipInfo":null
}
~~~~~~~~~~~~~

Status 401:
~~~~~~~~~~~~~{.js}
Client unathorized
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Room does not exist
~~~~~~~~~~~~~

### 3.2.4 Update Room {#RESTAPIsection3_2_4}

This API upate a room in current service.

**URL**

        http://<hostname>:3000/rooms/[room id]

**HTTP Method**

        PUT

**Parameters**

 - **Required**: A JSON string contains updated attributes. Example:
~~~~~~~~~~~~~{.js}
{
  "name": "updateRoom",
  "publishLimit": 10,
  "userLimit": 10,
}
~~~~~~~~~~~~~

**Response Example**

This API returns the updated room information.

Status 200:
~~~~~~~~~~~~~{.js}
{
  "_id":"57c67027596f657b1ef2c4f8"
  "name": "updateRoom",
  "mode":"hybrid",
  "publishLimit": 10,
  "userLimit": 10,
  "mediaMixing":{
    "video":{
      "avCoordinated":0,
      "maxInput":16,
      "resolution":"sif",
      "multistreaming":0,
      "bitrate":0,
      "bkColor":"black",
      "crop":0,
      "layout":{
        "base":"fluid",
        "custom":[]
      }
    },
    "audio":null
  },
  "enableMixing":1,
  "sipInfo":null
}
~~~~~~~~~~~~~

Status 400:
~~~~~~~~~~~~~{.js}
Bad room configuration
~~~~~~~~~~~~~

Status 401:
~~~~~~~~~~~~~{.js}
Client unathorized
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Room does not exist
~~~~~~~~~~~~~

### 3.2.5 Delete Room {#RESTAPIsection3_2_5}

This API delete a room in current service.

**URL**

        http://<hostname>:3000/rooms/[room id]

**HTTP Method**

        DELETE

**Response Example**

Status 200:
~~~~~~~~~~~~~{.js}
Room deleted
~~~~~~~~~~~~~

Status 401:
~~~~~~~~~~~~~{.js}
Client unathorized
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Room does not exist
~~~~~~~~~~~~~

## 3.3 User Management APIs {#RESTAPIsection3_3}

### 3.3.1 Create Token {#RESTAPIsection3_3_1}

This API creates a token for given user. Client can connect to specified room by this token.

**URL**

        http://<hostname>:3000/rooms/[room id]/tokens

**HTTP Method**

        POST

**Response Example**

Status 200:
~~~~~~~~~~~~~{.js}
eyJ0b2tlbklkIjoiNTNjZGNkMWU1Y2UzMTkxMjIyOWFjYTRjIiwiaG9zdCI6IjE5Mi4xNjguNDIuODg6ODA4MCIsInNpZ25hdHVyZSI6Ik1qQmhOVFZtWTJKa1pqZGhOMkk0T0RGa1lqTXhObUV5T1dVNE9USmtPVGt3TmpkbVpHRXdNUT09In0=
~~~~~~~~~~~~~

Status 401:
~~~~~~~~~~~~~{.js}
Name and role?
~~~~~~~~~~~~~

Status 401:
~~~~~~~~~~~~~{.js}
CloudHandler does not respond
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Service not found
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Room does not exist
~~~~~~~~~~~~~

**Remarks**

The token returned is encrypted with base 64. Client should decrypt it to get a JSON object. It has a property named host which is the hostname of the worker.

### 3.3.2 Get User List {#RESTAPIsection3_3_2}

This API lists all the users who joined the room of room ID.

**URL**

        http://<hostname>:3000/rooms/[room id]/users

**HTTP Method**

        GET

**Response Example**

Status 200:
~~~~~~~~~~~~~{.js}
[
{
  "name":"userA",
  "role":"presenter",
  "id":"/#XXmpJYPOZ9Ik2vTtAAAA"
},
{
  "name":"userB",
  "role":"viewer",
  "id":"/#jlw7oiBSK_xAPlqXAAAC"
}]
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Service not found
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Room not found
~~~~~~~~~~~~~

### 3.3.3 Get User {#RESTAPIsection3_3_3}

This API returns the users as requested.

**URL**

        http://<hostname>:3000/rooms/[room id]/users/[user name]

**HTTP Method**

        GET

**Response Example**

Status 200:
~~~~~~~~~~~~~{.js}
{
  "name":"userA",
  "role":"presenter",
  "id":"/#XXmpJYPOZ9Ik2vTtAAAA"
}
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Service not found
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Room does not exist
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
User does not exist
~~~~~~~~~~~~~

### 3.3.4 Delete User {#RESTAPIsection3_3_4}

This API deletes the user as requested.

**URL**

        http://<hostname>:3000/rooms/[room id]/users/[user name]

**HTTP Method**

        DELETE

**Response Example**

Status 200:
~~~~~~~~~~~~~{.js}
2 users deleted
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Service not found
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Room does not exist
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
User does not exist
~~~~~~~~~~~~~

Status 404:
~~~~~~~~~~~~~{.js}
Operation failed
~~~~~~~~~~~~~
