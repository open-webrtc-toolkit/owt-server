'use strict';

var Participant = function(spec, rpcReq) {
  var that = {},
    id = spec.id,
    role = spec.role,
    user = spec.user,
    portal = spec.portal,
    origin = spec.origin,
    permission = spec.permission;

  that.update = (op, path, value) => {
    switch (path) {
      case '/permission/subscribe':
        if (op === 'replace') {
          if (typeof value.audio === 'boolean' && typeof value.video === 'boolean') {
            permission.subscribe = value;
            return Promise.resolve('ok');
          } else {
            return Promise.reject('Invalid json value');
          }
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/subscribe/audio':
        if (op === 'replace') {
          if (typeof value === 'boolean') {
            permission.subscribe = (permission.subscribe || {audio: false, video: false});
            permission.subscribe.audio = value;
            return Promise.resolve('ok');
          } else {
            return Promise.reject('Invalid json value');
          }
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/subscribe/video':
        if (op === 'replace') {
          if (typeof value === 'boolean') {
            permission.subscribe = (permission.subscribe || {audio: false, video: false});
            permission.subscribe.video = value;
            return Promise.resolve('ok');
          } else {
            return Promise.reject('Invalid json value');
          }
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/publish':
        if (op === 'replace') {
          if (typeof value.audio === 'boolean' && typeof value.video === 'boolean') {
            permission.publish = value;
            return Promise.resolve('ok');
          } else {
            return Promise.reject('Invalid json value');
          }
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/publish/audio':
        if (op === 'replace') {
          if (typeof value === 'boolean') {
            permission.publish = (permission.publish || {audio: false, video: false});
            permission.publish.audio = value;
            return Promise.resolve('ok');
          } else {
            return Promise.reject('Invalid json value');
          }
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/publish/video':
        if (op === 'replace') {
          if (typeof value === 'boolean') {
            permission.publish = (permission.publish || {audio: false, video: false});
            permission.publish.video = value;
            return Promise.resolve('ok');
          } else {
            return Promise.reject('Invalid json value');
          }
        } else {
          return Promise.reject('Invalid json op');
        }
      default:
        return Promise.reject('Invalid json path');
    }
  };

  that.isPublishPermitted = (track) => {
    return !!(permission.publish && permission.publish[track]);
  };

  that.isSubscribePermitted = (track) => {
    return !!(permission.subscribe && permission.subscribe[track]);
  };

  that.notify = (event, data) => {
    return rpcReq.sendMsg(portal, id, event, data);
  };

  that.getInfo = () => {
    return {
      id: id,
      user: user,
      role: role,
    };
  };

  that.getDetail = () => {
    return {
      id: id,
      user: user,
      role: role,
      permission: permission
    };
  };

  that.getOrigin = () => {
    return origin;
  };

  that.getPortal = () => {
    return portal;
  };

  that.drop = () => {
    return rpcReq.dropUser(portal, id).catch(() => {});
  };

  return that;
};


module.exports = Participant;

