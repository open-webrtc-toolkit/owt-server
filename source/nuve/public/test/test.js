/*
* Copyright © 2015 Intel Corporation. All Rights Reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
* EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* global Nuve, console, prompt */
function setCookie (cname, cvalue, exdays) {
  'use strict';
  var d = new Date();
  d.setTime(d.getTime() + (exdays*24*60*60*1000));
  var expires = 'expires='+d.toUTCString();
  document.cookie = cname + '=' + cvalue + '; ' + expires;
}

function getCookie (cname) {
  'use strict';
  var name = cname + '=';
  var ca = document.cookie.split(';');
  for(var i=0; i<ca.length; i++) {
    var c = ca[i];
    while (c.charAt(0) == ' ') c = c.substring(1);
    if (c.indexOf(name) === 0) return c.substring(name.length, c.length);
  }
  return '';
}

(function () {
  'use strict';
  window.onload = function () {
    var serviceId = getCookie('serviceId');
    var serviceKey = getCookie('serviceKey');
    if (serviceId === '' || serviceKey === '') {
      serviceId = prompt('Enter Service Id:', '');
      serviceKey = prompt('Enter Service Key:', '');
    }
    setCookie('serviceId', serviceId, 365);
    setCookie('serviceKey', serviceKey, 365);

    var nuve = Nuve.init(serviceId, serviceKey);
    document.getElementById('sendButton').onclick = (function send () {
      document.getElementById('response').value = '';
      var callback = function (err, text) {
        if (err) return console.log(err, text);
        try {
          var obj = JSON.parse(text);
          console.table(obj);
          document.getElementById('response').value = JSON.stringify(obj, null, 2);
        } catch (e) {
          console.log(text);
          document.getElementById('response').value = text;
          return e;
        }
      };
      if (document.getElementById('createRoom').checked) {
        var room = {
          name: document.getElementById('room1').value,
          options: JSON.parse(document.getElementById('option1').value)
        };
        nuve.createRoom(room, callback);
      } else if (document.getElementById('getRooms').checked) {
        nuve.getRooms(callback);
      } else if (document.getElementById('getRoom').checked) {
        nuve.getRoom(document.getElementById('room1').value, callback);
      } else if (document.getElementById('deleteRoom').checked) {
        nuve.deleteRoom(document.getElementById('room1').value, callback);
      } else if (document.getElementById('getUsers').checked) {
        nuve.getUsers(document.getElementById('room1').value, callback);
      } else if (document.getElementById('updateRoom').checked) {
        nuve.updateRoom(document.getElementById('room1').value,
          JSON.parse(document.getElementById('option1').value),
          callback);
      } else if (document.getElementById('createService').checked) {
        nuve.createService(document.getElementById('service1').value,
          document.getElementById('key1').value, callback);
      } else if (document.getElementById('getServices').checked) {
        nuve.getServices(callback);
      } else if (document.getElementById('getService').checked) {
        nuve.getService(document.getElementById('service1').value, callback);
      } else if (document.getElementById('deleteService').checked) {
        nuve.deleteService(document.getElementById('service1').value, callback);
      } else if (document.getElementById('getClusterNodes').checked) {
        nuve.getClusterNodes(callback);
      } else if (document.getElementById('getClusterNode').checked) {
        nuve.getClusterNode(0, callback);
      } else if (document.getElementById('getClusterRooms').checked) {
        nuve.getClusterRooms(callback);
      }
    });
};
}());
