/* global require */
'use strict';

var Participant = function(spec, rpcReq) {
  var that = {},
    id = spec.id,
    role = spec.role,
    user = spec.user,
    portal = spec.portal,
    origin = spec.origin,
    permission = spec.permission;

  const updateArray = (target, op, value) => {
    if (op === 'add') {
      if (target.indexOf(value) === -1) {
        target.push(value);
        return Promise.resolve('ok');
      } else {
        return Promise.reject('Authority has already been there');
      }
    } else if (op === 'remove') {
      var i = target.indexOf(value);
      if (i !== -1) {
        target.splice(i, 1);
        return Promise.resolve('ok');
      } else {
        return Promise.reject('Authority is absent');
      }
    } else {
      return Promise.reject('Unknown op:', op);
    }
  };

  that.update = (op, path, value) => {
    switch (path) {
      case '/permission/subscribe':
        if (op === 'replace') {
          permission.subscribe = value;
          return Promise.resolve('ok');
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/subscribe/media':
        if (op === 'replace') {
          permission.subscribe = (permission.subscribe || {});
          permission.subscribe.media = value;
          return Promise.resolve('ok');
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/subscribe/media/audio':
        if (op === 'replace') {
          permission.subscribe = (permission.subscribe || {});
          permission.subscribe.media = (permission.subscribe.media || {});
          permission.subscribe.media.audio = value;
          return Promise.resolve('ok');
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/subscribe/media/video':
        if (op === 'replace') {
          permission.subscribe = (permission.subscribe || {});
          permission.subscribe.media = (permission.subscribe.media || {});
          permission.subscribe.media.video = value;
          return Promise.resolve('ok');
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/subscribe/type':
        return updateArray(permission.subscribe.type, op, value);
      case '/permission/publish':
        if (op === 'replace') {
          permission.publish = value;
          return Promise.resolve('ok');
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/publish/media':
        if (op === 'replace') {
          permission.publish = (permission.publish || {});
          permission.publish.media = value;
          return Promise.resolve('ok');
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/publish/media/audio':
        if (op === 'replace') {
          permission.publish = (permission.publish || {});
          permission.publish.media = (permission.publish.media || {});
          permission.publish.media.audio = value;
          return Promise.resolve('ok');
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/publish/media/video':
        if (op === 'replace') {
          permission.publish = (permission.publish || {});
          permission.publish.media = (permission.publish.media || {});
          permission.publish.media.video = value;
          return Promise.resolve('ok');
        } else {
          return Promise.reject('Invalid json op');
        }
      case '/permission/publish/type':
        return updateArray(permission.publish.type, op, value);
      default:
        return Promise.reject('Invalid json path');
    }
  };

  that.isPublishPermitted = (track, type) => {
    return !!((permission.publish === true)
              ||
              ((track === undefined || (permission.publish.media && (permission.publish.media[track] === true)))
               &&
               (type === undefined || (permission.publish.type && (permission.publish.type.indexOf(type) !== -1)))
              ));
  };

  that.isSubscribePermitted = (track, type) => {
    return !!((permission.subscribe === true)
              ||
              ((track === undefined || (permission.subscribe.media && (permission.subscribe.media[track] === true)))
               &&
               (type === undefined || (permission.subscribe.type && (permission.subscribe.type.indexOf(type) !== -1)))
              ));
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

