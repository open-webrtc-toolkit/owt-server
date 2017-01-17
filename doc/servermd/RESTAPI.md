Intel CS for WebRTC Management REST API
-------
# 1 Introduction {#RESTAPIsection1}
Intel WebRTC solution provides a set of REST (Representational State Transfer) API for conference management. Manager clients can be implemented by different programming languages through these APIs.

# 2 Definitions {#DEFINITIONS2}
Resource: the kind of data under manipulation
Verb: HTTP verb. The mapping to operations is as following:
 - Create operations: POST
 - Read operations: GET
 - Update operations: PUT
 - Delete operations: DELETE
URI: the Universal Resource Identifier, or name through which the resouce can be manipulated.
Request Body: the request data that user need supply to call the API. JSON format data is required.
Response Body: the response data is returned through HTTP response in JSON/text format.

# 3 Making a Call {#MAKINGACALL}
With all the management API, perform the following steps to implement each call.
1. Create a string for the http request that consists of the format [HTTP verb][url] + [headers] + [request body](optional)
2. Use your prefered language's network library (or http library) to send the request to REST server.
3. For authenication, REST API accept HTTP authentication on each request, see section4(Authentication and Authorization) for details.
4. Read the response in your programming language, and parse the response JSON data.
5. Use the data in your application.

The following HTTP response may be returned after your make a call:
200 - OK. Some calls return with a JSON format respone while some calls return a plain text response indicates success.
401 - Not authenticated or not permitted.
404 - Not found or no such resource.

# 4 Authentication and Authorization {#RESTAPIsection3}
The management API can only be called after authorized. To get authorized, each request should be a HTTP request with "Authorization" field in the header. So the server can determine whether it is a valid client. The value of "Authorization" is something like:
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

```toSign``` is the signature.

**Note:
If authentication failed, a response with code 401 would be returned.
~~~~~~~~~~~~~{.js}
{'WWW-Authenticate': 'MAuth realm="http://marte3.dit.upm.es"'}
~~~~~~~~~~~~~

# 5 Mangement Resource {#RESTAPIsection5}
The management API uses REST model to accessing different resources. The resources in conference management can be grouped into 4 kinds:
 - Service
 - Room
 - Token
 - User
Note: Once you use a resource, concatenate it to your server's IP or hostname to form the url.

## 5.1 Service
Description:
  The service represents the host of a set of rooms. Each service has its own identifier. The service ID and key are required to generate authentication header for HTTP requests. Note that there is one super service whose ID can be specified by nuve.toml. The service resource can only be manipulated under super service authentication while other resouces require corresponding host service's authentication.

Data Model:
Service data JSON example:
~~~~~~~~~~~~~{.js}
{
  "_id" : "MyServiceID",          // The ID of the service
  "name" : "MyServiceName",       // The name of the service
  "key" : "MyServiceKey",         // The key of the service
  "encrypted" : true,             // Encrypted or not
  "rooms" : [...],                // The list of room data under this service
}
~~~~~~~~~~~~~

Resources:
 - /services
 - /services/{serviceId}

### /services
#### GET
___
Read all the services.

Parameters
None.

Response Body
Type - JSON.
Content - List of service data.
Example: [service0, ...]
(elements in list are of service data model.)

#### POST
___
Create a service.

Parameters
Path:
None.
Query:
A JSON object describes the service to be created.
~~~~~~~~~~~~~{.js}
{
  "name" : "MyServiceName", // The name of the service to create
  "key" : "MyServiceKey",   // The key of the service to create
}
~~~~~~~~~~~~~

Response Body
Type - JSON.
Content - A service data model.

### /services/{serviceId}
#### GET
___
Read the service specified by ID.

Parameters
Path:
serviceId - The specified service ID.
Query:
None.

Response Body
Type - JSON.
Content - The required service data model.

#### DELETE
___
Delete the service specified by ID.

Parameters
Path:
serviceId - The specified service ID.
Query:
None.

Response Body
Type - Text.
Content - A text message "Service deleted" indicates success.

## 5.2 Room
Description:
  A room is the conference where users can communicate by real-time video and audio. Rooms are served by services. These resources can only be accessed after authenticated to their owner services. Rooms can have different settings for various scenarios. Making call on room resources provides users the ability to customize rooms for different needs.

Data Model:
Room data JSON example:
~~~~~~~~~~~~~{.js}
{
  "_id":"MyRoomID",                  // The ID of the room.
  "name":"MyRoom",                   // The name of the room.
  "mode":"hybrid",                   // The room mode, only "hybrid" supported.
  "publishLimit":-1,                 // The number of published streams limit, -1 indicates no limit.
  "userLimit":-1,                    // The number of users in this room limit.
  "enableMixing":1,                  // Enable mixing settings for room, 1 for yes, 0 for no.
  "mediaMixing":{                    // Settings for mix stream, only works when enableMixing is 1.
    "video":{                        // Setting for mix video.
      "avCoordinated":0,             // Enable VAD, 1 for yes, 0 for no.
      "maxInput":16,                 // Max input for mix video, -1 indicates no limit.
      "resolution":"vga",            // The mix video's resolution, if multistreaming is 1, this specifies the maximum resolution.
      "multistreaming":0,            // Enable multistreaming, 1 for yes, 0 for no.
      "quality_level":"standard",    // Quality level of mix video, not supported.
      "bkColor":"black",             // The background color of the mix video.
      "crop":0,                      // Whether crop for mix video, 1 for yes, 0 for no.
      "layout":{                     // The layout of mix video.
        "base":"fluid",              // Base layout, values in ["fluid", "lecture", "void"]
        "custom":[]                  // Custom layout setting, take affact when base is "void", details are described in conference guide.
      }
    },
    "audio":null                     // Mix audio setting not supported.
  },
  "sipInfo":{                        // The sip connectivity setting for room, set this to null when disabled.
    "sipServer":"my.sip.server",     // The sip server's IP or hostname.
    "username":"sipuser",            // The sip user name for this room.
    "password":"sippassword"         // The sip password for this room.
  }
}
~~~~~~~~~~~~~

Resources:
 - /rooms
 - /rooms/{roomId}

### /rooms
#### GET
___
Read all the rooms using current service's authentication.

Parameters
None.

Response Body
Type - JSON.
Content - List of room data.
Example: [room0, ...]
(elements in list are of room data model.)

#### POST
___
Create a room using current service's authentication.

Parameters
Path:
None.
Query:
A JSON object describes the room to be created.
~~~~~~~~~~~~~{.js}
{
  "name": "apiRoom",                    // Required, type string.
  options: {                            // The attributes in options are all optional.
    "mode": "hybrid",                   // Optional, type string, only support "hybrid" in current release.
    "publishLimit": 16,                 // Optional, type number, -1 means unlimited.
    "userLimit": 100,                   // Optional, type number, -1 means unlimited.
    "enableMixing": 1,                  // Optional, type number: 0/1.
    "mediaMixing": {                    // Optional, type object.
      "video": {
        "resolution": "vga",            // Optional, type string, valid values ["vga", "sif", "xga", "svga", "hd720p", "hd1080p", "uhd_4k", "r720x720"].
        "bitrate": 0,                   // Optional, type number.
        "bkColor": {
          "r": 255,                     // Required if bkColor specified, type number, 0 ~ 255.
          "g": 255,                     // Required if bkColor specified, type number, 0 ~ 255.
          "b": 255                      // Required if bkColor specified, type number, 0 ~ 255.
        },
        "maxInput": 16,                 // Optional, type number.
        "avCoordinated": 0,             // Optional, type number: 0/1.
        "multistreaming": 0,            // Optional, type number: 0/1.
        "crop": 0,                      // Optional, type number: 0/1.
        "layout": {                     // Optional, type object.
          "base": "fluid",              // Optional, type string, valid values ["fluid", "lecture", "void"].
          "custom": []                  // Required if base is "void", type object::Array or null, the format is introduced in MCU server guide.
        }
      },
      "audio": null                     // Optional, only null supported.
    },
    "sipInfo":{                         // Optional, type object.
      "sipServer":"my.sip.server",      // Required if sipInfo specified, type string.
      "username":"imUser",              // Required if sipInfo specified, type string.
      "password":"imPassword"           // Required if sipInfo specified, type string.
    }
  }
}
~~~~~~~~~~~~~

Response Body
Type - JSON.
Content - A room data model represent the room created.

### /rooms/{roomId}
#### GET
___
Read the room specified by ID under current service.

Parameters
Path:
roomId - The specified room ID.
Query:
A JSON object describes the room updates contains attributes(except id) in room data model.

Response Body
Type - JSON.
Content - The required room data model.

#### PUT
___
Update the room specified by ID under current service.

Parameters
Path:
roomId - The specified room ID.
Query:
None.

Response Body
Type - JSON.
Content - The updated room data model.


#### DELETE
___
Delete the room specified by ID under current service.

Parameters
Path:
roomId - The specified room ID.
Query:
None.

Response Body
Type - Text.
Content - A text message "Room deleted" indicates success.

## 5.3 Token
Description:
  A token is the ticket for joining the room. The token contains information through which clients can connect to server application. Note that the different rooms may have different network information, so the room must be specified for token resource. The same token cannot be reused if it has been used once. Re-generate token if clients need to connect at the second time.

Data Model:
Token data in JSON example:
~~~~~~~~~~~~~{.js}
{
  tokenId: "tokenID",
  host: "roomHost",
  secure: "secureOrNot",
  signature: "signature"
}
~~~~~~~~~~~~~

Resources:
 - /rooms/{roomId}/tokens
 - /rooms/{roomId}/tokens/{type}

### /rooms/{roomId}/tokens
#### POST
___
Create a socket.io token for specified room using current service's authentication. Note that "mauth_username" and "mauth_role" should be added in "Authorization" field of the request header to determine the user name and role (Refer to section "Authentication and Authorization").

Parameters
Path:
roomId - The specified room ID.
Query:
None.

Response Body
Type - Text.
Content - Base64 encrypted text of JSON token data model.

### /rooms/{roomId}/tokens/{type}
#### POST
___
Create a socket.io or rest token for specified room using current service's authentication. Similar to the POST /rooms/{roomId}/tokens, "mauth_username" and "mauth_role" are also required in "Authorization" header.

Parameters
Path:
roomId - The specified room ID.
type - Token type, "socketio" or "rest".
Query:
None.

Response Body
Type - Text.
Content - Base64 encrypted text of JSON token data model.


## 5.4 User
Description:
  User is the representation for clients in the room. This resource provides the ability for querying and managing clients in the spcefied room. Note that the user resource does not have a adding-user interface. The user can only be added by joining the room through client sdk with a token created by the management.

Data Model:
User data JSON example:
~~~~~~~~~~~~~{.js}
{
  "name":"MyUserName",  // The user name.
  "role":"MyRole",      // The user role, related to the server configuration.
  "id":"MyUserID"       // The user ID.
}
~~~~~~~~~~~~~

Resources:
 - /rooms/{roomId}/users
 - /rooms/{roomId}/users/{userId}

### /rooms/{roomId}/users
#### GET
___
Read all the users in the specified room using current service's authentication.

Parameters
Path:
roomId - The specified room ID.
Query:
None.

Response Body
Type - JSON.
Content - List of user data.
Example: [user0, ...]
(elements in list are of user data model.)

### /rooms/{roomId}/users/{userId}
#### GET
___
Read the user specified by ID in the specified room under current service.

Parameters
Path:
roomId - The specified room ID.
userId - The specified user ID.
Query:
None.

Response Body
Type - JSON.
Content - The required user data model.

#### DELETE
___
Delete the room specified by ID under current service.

Parameters
Path:
roomId - The specified room ID.
userId - The specified user ID.
Query:
None.

Response Body
Type - Text.
Content - A text message "User deleted" indicates success.
