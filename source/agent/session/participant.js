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

  const setPubSubAuthorigy = (operation, field, value) => {
    if (field === 'media.audio' || field === 'media.video') {
      var track = field.substring(6);
      if (typeof value === 'boolean') {
        permission[operation].media || (permission[operation].media = {});
        permission[operation].media[track] = value;
        return Promise.resolve('ok');
      } else {
        return Promise.reject('Invalid authority value: ' + value);
      }
    } else if (field === 'type.add') {
      var type = value;
      if (type === 'webrtc' || type === 'streaming' || type === 'recording') {
        permission[operation].type || (permission[operation].type = []);
        (permission[operation].type.indexOf(type) === -1) && (permission[operation].type.push(type));
        return Promise.resolve('ok');
      } else {
        return Promise.reject('Invalid authority value: ' + value);
      }
    } else if (field === 'type.remove') {
      var type = value;
      if (type === 'webrtc' || type === 'streaming' || type === 'recording') {
        permission[operation].type && (permission[operation].type.indexOf(type) !== -1) && (permission[operation].type.splice(permission[operation].type.indexOf(type), 1));
        return Promise.resolve('ok');
      } else {
        return Promise.reject('Invalid authority value: ' + value);
      }
    } else {
      return Promise.reject('Invalid authority field: ' + field);
    }
  };

  const setTextPermission = (value) => {
    if ((value === 'to-all') || (value === 'to-peer') || (value === false)) {
      permission.text = value;
      return Promise.resolve('ok');
    } else {
      return Promise.reject('Invalid text authority value: ' + value);
    }
  };

  const setManagePermission = (value) => {
    if (typeof value === 'boolean') {
      permission.manage = value;
      return Promise.resolve('ok');
    } else {
      return Promise.reject('Invalide manage authority value: ' + value);
    }
  };

  that.setPermission = (operation, field, value) => {
    switch (operation) {
      case 'publish':
      case 'subscribe':
        return setPubSubAuthorigy(operation, field, value);
      case 'text':
        return setTextPermission(value);
      case 'manage':
        return setManagePermission(value);
      default:
        return Promise.reject('Unknown operation: ' + operation);
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

  that.isManagePermitted = () => {
    return !!permission.manage;
  };

  that.isTextPermitted = function(to) {
    return !!(
              (permission.text !== false)
              &&
              ((to !== 'all') || (permission.text === 'to-all'))
             );
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

  that.getOrigin = () => {
    return origin;
  };

  that.getPortal = () => {
    return portal;
  };

  return that;
};


module.exports = Participant;

