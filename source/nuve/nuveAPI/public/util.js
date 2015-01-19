/* global PNotify, $ */
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

function notify(level, title, message) {
  'use strict';
  $(function(){
    new PNotify({
      title: title,
      text: message,
      opacity: 0.8,
      type: level
    });
  });
}

function notifyConfirm (title, message, onConfirm, onCancle) {
  'use strict';
  (new PNotify({
    title: title,
    text: message,
    icon: 'glyphicon glyphicon-question-sign',
    hide: false,
    confirm: {
      confirm: true
    },
    buttons: {
      closer: false,
      sticker: false
    },
    history: {
      history: false
    }
  })).get().on('pnotify.confirm', onConfirm).on('pnotify.cancel', onCancle);
}
