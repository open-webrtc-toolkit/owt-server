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
  }

  data() {
    return {
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
      if (message.toLowerCase().indexOf('service')) code = 1002;
      if (message.toLowerCase().indexOf('room')) code = 1003;
      if (message.toLowerCase().indexOf('stream')) code = 1004;
      if (message.toLowerCase().indexOf('participant')) code = 1005;
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