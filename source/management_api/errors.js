// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

// Base class for all app errors
class AppError extends Error {
  constructor (message, status, code) {
    super(message);
    this.name = this.constructor.name;
    // Capturing stack trace, excluding constructor call from it.
    Error.captureStackTrace(this, this.constructor);
    // Default status
    this.status = status || 500;
    this.code = code || 2001;
    this.data = {
      error: {
        code: this.code,
        message: this.message
      }
    };
  }
}

/*
 * Internal Error Code
 * General not found - 1001
 * Service not found - 1002
 * Room not found - 1003
 * Stream not found - 1004
 * Participant not found - 1005
 */
class NotFoundError extends AppError {
  constructor (message, code) {
    if (!code) {
      code = 1001;
      if (typeof message === 'string') {
        if (message.toLowerCase().indexOf('service') >= 0) code = 1002;
        if (message.toLowerCase().indexOf('room') >= 0) code = 1003;
        if (message.toLowerCase().indexOf('stream') >= 0) code = 1004;
        if (message.toLowerCase().indexOf('participant') >= 0) code = 1005;
      }
    }
    super(message || 'Resource not found', 404, code);
  }
}

class AuthError extends AppError {
  constructor (message, code) {
    super(message || 'Authentication failed', 401, code || 1101);
  }
}

class AccessError extends AppError {
  constructor (message, code) {
    super(message || 'Access forbiden', 403, code || 1102);
  }
}

class BadRequestError extends AppError {
  constructor (message, code) {
    super(message || 'Bad request', 400, code || 1201);
  }
}

class CloudError extends AppError {
  constructor (message, code) {
    super(message || 'Cloud handler failed', 500, code || 1301);
  }
}

module.exports = {
  AppError,
  NotFoundError,
  AuthError,
  AccessError,
  CloudError,
  BadRequestError
};