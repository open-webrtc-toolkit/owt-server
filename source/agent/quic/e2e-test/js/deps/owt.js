(function(f){if(typeof exports==="object"&&typeof module!=="undefined"){module.exports=f()}else if(typeof define==="function"&&define.amd){define([],f)}else{var g;if(typeof window!=="undefined"){g=window}else if(typeof global!=="undefined"){g=global}else if(typeof self!=="undefined"){g=self}else{g=this}g.Owt = f()}})(function(){var define,module,exports;return (function(){function r(e,n,t){function o(i,f){if(!n[i]){if(!e[i]){var c="function"==typeof require&&require;if(!f&&c)return c(i,!0);if(u)return u(i,!0);var a=new Error("Cannot find module '"+i+"'");throw a.code="MODULE_NOT_FOUND",a}var p=n[i]={exports:{}};e[i][0].call(p.exports,function(r){var n=e[i][1][r];return o(n||r)},p,p.exports,r,e,n,t)}return n[i].exports}for(var u="function"==typeof require&&require,i=0;i<t.length;i++)o(t[i]);return o}return r})()({1:[function(require,module,exports){
function _arrayLikeToArray(arr, len) {
  if (len == null || len > arr.length) len = arr.length;

  for (var i = 0, arr2 = new Array(len); i < len; i++) {
    arr2[i] = arr[i];
  }

  return arr2;
}

module.exports = _arrayLikeToArray;
},{}],2:[function(require,module,exports){
function _arrayWithHoles(arr) {
  if (Array.isArray(arr)) return arr;
}

module.exports = _arrayWithHoles;
},{}],3:[function(require,module,exports){
function _assertThisInitialized(self) {
  if (self === void 0) {
    throw new ReferenceError("this hasn't been initialised - super() hasn't been called");
  }

  return self;
}

module.exports = _assertThisInitialized;
},{}],4:[function(require,module,exports){
function asyncGeneratorStep(gen, resolve, reject, _next, _throw, key, arg) {
  try {
    var info = gen[key](arg);
    var value = info.value;
  } catch (error) {
    reject(error);
    return;
  }

  if (info.done) {
    resolve(value);
  } else {
    Promise.resolve(value).then(_next, _throw);
  }
}

function _asyncToGenerator(fn) {
  return function () {
    var self = this,
        args = arguments;
    return new Promise(function (resolve, reject) {
      var gen = fn.apply(self, args);

      function _next(value) {
        asyncGeneratorStep(gen, resolve, reject, _next, _throw, "next", value);
      }

      function _throw(err) {
        asyncGeneratorStep(gen, resolve, reject, _next, _throw, "throw", err);
      }

      _next(undefined);
    });
  };
}

module.exports = _asyncToGenerator;
},{}],5:[function(require,module,exports){
function _classCallCheck(instance, Constructor) {
  if (!(instance instanceof Constructor)) {
    throw new TypeError("Cannot call a class as a function");
  }
}

module.exports = _classCallCheck;
},{}],6:[function(require,module,exports){
var setPrototypeOf = require("./setPrototypeOf");

var isNativeReflectConstruct = require("./isNativeReflectConstruct");

function _construct(Parent, args, Class) {
  if (isNativeReflectConstruct()) {
    module.exports = _construct = Reflect.construct;
  } else {
    module.exports = _construct = function _construct(Parent, args, Class) {
      var a = [null];
      a.push.apply(a, args);
      var Constructor = Function.bind.apply(Parent, a);
      var instance = new Constructor();
      if (Class) setPrototypeOf(instance, Class.prototype);
      return instance;
    };
  }

  return _construct.apply(null, arguments);
}

module.exports = _construct;
},{"./isNativeReflectConstruct":13,"./setPrototypeOf":17}],7:[function(require,module,exports){
function _defineProperties(target, props) {
  for (var i = 0; i < props.length; i++) {
    var descriptor = props[i];
    descriptor.enumerable = descriptor.enumerable || false;
    descriptor.configurable = true;
    if ("value" in descriptor) descriptor.writable = true;
    Object.defineProperty(target, descriptor.key, descriptor);
  }
}

function _createClass(Constructor, protoProps, staticProps) {
  if (protoProps) _defineProperties(Constructor.prototype, protoProps);
  if (staticProps) _defineProperties(Constructor, staticProps);
  return Constructor;
}

module.exports = _createClass;
},{}],8:[function(require,module,exports){
function _getPrototypeOf(o) {
  module.exports = _getPrototypeOf = Object.setPrototypeOf ? Object.getPrototypeOf : function _getPrototypeOf(o) {
    return o.__proto__ || Object.getPrototypeOf(o);
  };
  return _getPrototypeOf(o);
}

module.exports = _getPrototypeOf;
},{}],9:[function(require,module,exports){
var setPrototypeOf = require("./setPrototypeOf");

function _inherits(subClass, superClass) {
  if (typeof superClass !== "function" && superClass !== null) {
    throw new TypeError("Super expression must either be null or a function");
  }

  subClass.prototype = Object.create(superClass && superClass.prototype, {
    constructor: {
      value: subClass,
      writable: true,
      configurable: true
    }
  });
  if (superClass) setPrototypeOf(subClass, superClass);
}

module.exports = _inherits;
},{"./setPrototypeOf":17}],10:[function(require,module,exports){
function _interopRequireDefault(obj) {
  return obj && obj.__esModule ? obj : {
    "default": obj
  };
}

module.exports = _interopRequireDefault;
},{}],11:[function(require,module,exports){
var _typeof = require("@babel/runtime/helpers/typeof");

function _getRequireWildcardCache() {
  if (typeof WeakMap !== "function") return null;
  var cache = new WeakMap();

  _getRequireWildcardCache = function _getRequireWildcardCache() {
    return cache;
  };

  return cache;
}

function _interopRequireWildcard(obj) {
  if (obj && obj.__esModule) {
    return obj;
  }

  if (obj === null || _typeof(obj) !== "object" && typeof obj !== "function") {
    return {
      "default": obj
    };
  }

  var cache = _getRequireWildcardCache();

  if (cache && cache.has(obj)) {
    return cache.get(obj);
  }

  var newObj = {};
  var hasPropertyDescriptor = Object.defineProperty && Object.getOwnPropertyDescriptor;

  for (var key in obj) {
    if (Object.prototype.hasOwnProperty.call(obj, key)) {
      var desc = hasPropertyDescriptor ? Object.getOwnPropertyDescriptor(obj, key) : null;

      if (desc && (desc.get || desc.set)) {
        Object.defineProperty(newObj, key, desc);
      } else {
        newObj[key] = obj[key];
      }
    }
  }

  newObj["default"] = obj;

  if (cache) {
    cache.set(obj, newObj);
  }

  return newObj;
}

module.exports = _interopRequireWildcard;
},{"@babel/runtime/helpers/typeof":19}],12:[function(require,module,exports){
function _isNativeFunction(fn) {
  return Function.toString.call(fn).indexOf("[native code]") !== -1;
}

module.exports = _isNativeFunction;
},{}],13:[function(require,module,exports){
function _isNativeReflectConstruct() {
  if (typeof Reflect === "undefined" || !Reflect.construct) return false;
  if (Reflect.construct.sham) return false;
  if (typeof Proxy === "function") return true;

  try {
    Date.prototype.toString.call(Reflect.construct(Date, [], function () {}));
    return true;
  } catch (e) {
    return false;
  }
}

module.exports = _isNativeReflectConstruct;
},{}],14:[function(require,module,exports){
function _iterableToArrayLimit(arr, i) {
  if (typeof Symbol === "undefined" || !(Symbol.iterator in Object(arr))) return;
  var _arr = [];
  var _n = true;
  var _d = false;
  var _e = undefined;

  try {
    for (var _i = arr[Symbol.iterator](), _s; !(_n = (_s = _i.next()).done); _n = true) {
      _arr.push(_s.value);

      if (i && _arr.length === i) break;
    }
  } catch (err) {
    _d = true;
    _e = err;
  } finally {
    try {
      if (!_n && _i["return"] != null) _i["return"]();
    } finally {
      if (_d) throw _e;
    }
  }

  return _arr;
}

module.exports = _iterableToArrayLimit;
},{}],15:[function(require,module,exports){
function _nonIterableRest() {
  throw new TypeError("Invalid attempt to destructure non-iterable instance.\nIn order to be iterable, non-array objects must have a [Symbol.iterator]() method.");
}

module.exports = _nonIterableRest;
},{}],16:[function(require,module,exports){
var _typeof = require("@babel/runtime/helpers/typeof");

var assertThisInitialized = require("./assertThisInitialized");

function _possibleConstructorReturn(self, call) {
  if (call && (_typeof(call) === "object" || typeof call === "function")) {
    return call;
  }

  return assertThisInitialized(self);
}

module.exports = _possibleConstructorReturn;
},{"./assertThisInitialized":3,"@babel/runtime/helpers/typeof":19}],17:[function(require,module,exports){
function _setPrototypeOf(o, p) {
  module.exports = _setPrototypeOf = Object.setPrototypeOf || function _setPrototypeOf(o, p) {
    o.__proto__ = p;
    return o;
  };

  return _setPrototypeOf(o, p);
}

module.exports = _setPrototypeOf;
},{}],18:[function(require,module,exports){
var arrayWithHoles = require("./arrayWithHoles");

var iterableToArrayLimit = require("./iterableToArrayLimit");

var unsupportedIterableToArray = require("./unsupportedIterableToArray");

var nonIterableRest = require("./nonIterableRest");

function _slicedToArray(arr, i) {
  return arrayWithHoles(arr) || iterableToArrayLimit(arr, i) || unsupportedIterableToArray(arr, i) || nonIterableRest();
}

module.exports = _slicedToArray;
},{"./arrayWithHoles":2,"./iterableToArrayLimit":14,"./nonIterableRest":15,"./unsupportedIterableToArray":20}],19:[function(require,module,exports){
function _typeof(obj) {
  "@babel/helpers - typeof";

  if (typeof Symbol === "function" && typeof Symbol.iterator === "symbol") {
    module.exports = _typeof = function _typeof(obj) {
      return typeof obj;
    };
  } else {
    module.exports = _typeof = function _typeof(obj) {
      return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj;
    };
  }

  return _typeof(obj);
}

module.exports = _typeof;
},{}],20:[function(require,module,exports){
var arrayLikeToArray = require("./arrayLikeToArray");

function _unsupportedIterableToArray(o, minLen) {
  if (!o) return;
  if (typeof o === "string") return arrayLikeToArray(o, minLen);
  var n = Object.prototype.toString.call(o).slice(8, -1);
  if (n === "Object" && o.constructor) n = o.constructor.name;
  if (n === "Map" || n === "Set") return Array.from(o);
  if (n === "Arguments" || /^(?:Ui|I)nt(?:8|16|32)(?:Clamped)?Array$/.test(n)) return arrayLikeToArray(o, minLen);
}

module.exports = _unsupportedIterableToArray;
},{"./arrayLikeToArray":1}],21:[function(require,module,exports){
var getPrototypeOf = require("./getPrototypeOf");

var setPrototypeOf = require("./setPrototypeOf");

var isNativeFunction = require("./isNativeFunction");

var construct = require("./construct");

function _wrapNativeSuper(Class) {
  var _cache = typeof Map === "function" ? new Map() : undefined;

  module.exports = _wrapNativeSuper = function _wrapNativeSuper(Class) {
    if (Class === null || !isNativeFunction(Class)) return Class;

    if (typeof Class !== "function") {
      throw new TypeError("Super expression must either be null or a function");
    }

    if (typeof _cache !== "undefined") {
      if (_cache.has(Class)) return _cache.get(Class);

      _cache.set(Class, Wrapper);
    }

    function Wrapper() {
      return construct(Class, arguments, getPrototypeOf(this).constructor);
    }

    Wrapper.prototype = Object.create(Class.prototype, {
      constructor: {
        value: Wrapper,
        enumerable: false,
        writable: true,
        configurable: true
      }
    });
    return setPrototypeOf(Wrapper, Class);
  };

  return _wrapNativeSuper(Class);
}

module.exports = _wrapNativeSuper;
},{"./construct":6,"./getPrototypeOf":8,"./isNativeFunction":12,"./setPrototypeOf":17}],22:[function(require,module,exports){
module.exports = require("regenerator-runtime");

},{"regenerator-runtime":23}],23:[function(require,module,exports){
/**
 * Copyright (c) 2014-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

var runtime = (function (exports) {
  "use strict";

  var Op = Object.prototype;
  var hasOwn = Op.hasOwnProperty;
  var undefined; // More compressible than void 0.
  var $Symbol = typeof Symbol === "function" ? Symbol : {};
  var iteratorSymbol = $Symbol.iterator || "@@iterator";
  var asyncIteratorSymbol = $Symbol.asyncIterator || "@@asyncIterator";
  var toStringTagSymbol = $Symbol.toStringTag || "@@toStringTag";

  function define(obj, key, value) {
    Object.defineProperty(obj, key, {
      value: value,
      enumerable: true,
      configurable: true,
      writable: true
    });
    return obj[key];
  }
  try {
    // IE 8 has a broken Object.defineProperty that only works on DOM objects.
    define({}, "");
  } catch (err) {
    define = function(obj, key, value) {
      return obj[key] = value;
    };
  }

  function wrap(innerFn, outerFn, self, tryLocsList) {
    // If outerFn provided and outerFn.prototype is a Generator, then outerFn.prototype instanceof Generator.
    var protoGenerator = outerFn && outerFn.prototype instanceof Generator ? outerFn : Generator;
    var generator = Object.create(protoGenerator.prototype);
    var context = new Context(tryLocsList || []);

    // The ._invoke method unifies the implementations of the .next,
    // .throw, and .return methods.
    generator._invoke = makeInvokeMethod(innerFn, self, context);

    return generator;
  }
  exports.wrap = wrap;

  // Try/catch helper to minimize deoptimizations. Returns a completion
  // record like context.tryEntries[i].completion. This interface could
  // have been (and was previously) designed to take a closure to be
  // invoked without arguments, but in all the cases we care about we
  // already have an existing method we want to call, so there's no need
  // to create a new function object. We can even get away with assuming
  // the method takes exactly one argument, since that happens to be true
  // in every case, so we don't have to touch the arguments object. The
  // only additional allocation required is the completion record, which
  // has a stable shape and so hopefully should be cheap to allocate.
  function tryCatch(fn, obj, arg) {
    try {
      return { type: "normal", arg: fn.call(obj, arg) };
    } catch (err) {
      return { type: "throw", arg: err };
    }
  }

  var GenStateSuspendedStart = "suspendedStart";
  var GenStateSuspendedYield = "suspendedYield";
  var GenStateExecuting = "executing";
  var GenStateCompleted = "completed";

  // Returning this object from the innerFn has the same effect as
  // breaking out of the dispatch switch statement.
  var ContinueSentinel = {};

  // Dummy constructor functions that we use as the .constructor and
  // .constructor.prototype properties for functions that return Generator
  // objects. For full spec compliance, you may wish to configure your
  // minifier not to mangle the names of these two functions.
  function Generator() {}
  function GeneratorFunction() {}
  function GeneratorFunctionPrototype() {}

  // This is a polyfill for %IteratorPrototype% for environments that
  // don't natively support it.
  var IteratorPrototype = {};
  IteratorPrototype[iteratorSymbol] = function () {
    return this;
  };

  var getProto = Object.getPrototypeOf;
  var NativeIteratorPrototype = getProto && getProto(getProto(values([])));
  if (NativeIteratorPrototype &&
      NativeIteratorPrototype !== Op &&
      hasOwn.call(NativeIteratorPrototype, iteratorSymbol)) {
    // This environment has a native %IteratorPrototype%; use it instead
    // of the polyfill.
    IteratorPrototype = NativeIteratorPrototype;
  }

  var Gp = GeneratorFunctionPrototype.prototype =
    Generator.prototype = Object.create(IteratorPrototype);
  GeneratorFunction.prototype = Gp.constructor = GeneratorFunctionPrototype;
  GeneratorFunctionPrototype.constructor = GeneratorFunction;
  GeneratorFunction.displayName = define(
    GeneratorFunctionPrototype,
    toStringTagSymbol,
    "GeneratorFunction"
  );

  // Helper for defining the .next, .throw, and .return methods of the
  // Iterator interface in terms of a single ._invoke method.
  function defineIteratorMethods(prototype) {
    ["next", "throw", "return"].forEach(function(method) {
      define(prototype, method, function(arg) {
        return this._invoke(method, arg);
      });
    });
  }

  exports.isGeneratorFunction = function(genFun) {
    var ctor = typeof genFun === "function" && genFun.constructor;
    return ctor
      ? ctor === GeneratorFunction ||
        // For the native GeneratorFunction constructor, the best we can
        // do is to check its .name property.
        (ctor.displayName || ctor.name) === "GeneratorFunction"
      : false;
  };

  exports.mark = function(genFun) {
    if (Object.setPrototypeOf) {
      Object.setPrototypeOf(genFun, GeneratorFunctionPrototype);
    } else {
      genFun.__proto__ = GeneratorFunctionPrototype;
      define(genFun, toStringTagSymbol, "GeneratorFunction");
    }
    genFun.prototype = Object.create(Gp);
    return genFun;
  };

  // Within the body of any async function, `await x` is transformed to
  // `yield regeneratorRuntime.awrap(x)`, so that the runtime can test
  // `hasOwn.call(value, "__await")` to determine if the yielded value is
  // meant to be awaited.
  exports.awrap = function(arg) {
    return { __await: arg };
  };

  function AsyncIterator(generator, PromiseImpl) {
    function invoke(method, arg, resolve, reject) {
      var record = tryCatch(generator[method], generator, arg);
      if (record.type === "throw") {
        reject(record.arg);
      } else {
        var result = record.arg;
        var value = result.value;
        if (value &&
            typeof value === "object" &&
            hasOwn.call(value, "__await")) {
          return PromiseImpl.resolve(value.__await).then(function(value) {
            invoke("next", value, resolve, reject);
          }, function(err) {
            invoke("throw", err, resolve, reject);
          });
        }

        return PromiseImpl.resolve(value).then(function(unwrapped) {
          // When a yielded Promise is resolved, its final value becomes
          // the .value of the Promise<{value,done}> result for the
          // current iteration.
          result.value = unwrapped;
          resolve(result);
        }, function(error) {
          // If a rejected Promise was yielded, throw the rejection back
          // into the async generator function so it can be handled there.
          return invoke("throw", error, resolve, reject);
        });
      }
    }

    var previousPromise;

    function enqueue(method, arg) {
      function callInvokeWithMethodAndArg() {
        return new PromiseImpl(function(resolve, reject) {
          invoke(method, arg, resolve, reject);
        });
      }

      return previousPromise =
        // If enqueue has been called before, then we want to wait until
        // all previous Promises have been resolved before calling invoke,
        // so that results are always delivered in the correct order. If
        // enqueue has not been called before, then it is important to
        // call invoke immediately, without waiting on a callback to fire,
        // so that the async generator function has the opportunity to do
        // any necessary setup in a predictable way. This predictability
        // is why the Promise constructor synchronously invokes its
        // executor callback, and why async functions synchronously
        // execute code before the first await. Since we implement simple
        // async functions in terms of async generators, it is especially
        // important to get this right, even though it requires care.
        previousPromise ? previousPromise.then(
          callInvokeWithMethodAndArg,
          // Avoid propagating failures to Promises returned by later
          // invocations of the iterator.
          callInvokeWithMethodAndArg
        ) : callInvokeWithMethodAndArg();
    }

    // Define the unified helper method that is used to implement .next,
    // .throw, and .return (see defineIteratorMethods).
    this._invoke = enqueue;
  }

  defineIteratorMethods(AsyncIterator.prototype);
  AsyncIterator.prototype[asyncIteratorSymbol] = function () {
    return this;
  };
  exports.AsyncIterator = AsyncIterator;

  // Note that simple async functions are implemented on top of
  // AsyncIterator objects; they just return a Promise for the value of
  // the final result produced by the iterator.
  exports.async = function(innerFn, outerFn, self, tryLocsList, PromiseImpl) {
    if (PromiseImpl === void 0) PromiseImpl = Promise;

    var iter = new AsyncIterator(
      wrap(innerFn, outerFn, self, tryLocsList),
      PromiseImpl
    );

    return exports.isGeneratorFunction(outerFn)
      ? iter // If outerFn is a generator, return the full iterator.
      : iter.next().then(function(result) {
          return result.done ? result.value : iter.next();
        });
  };

  function makeInvokeMethod(innerFn, self, context) {
    var state = GenStateSuspendedStart;

    return function invoke(method, arg) {
      if (state === GenStateExecuting) {
        throw new Error("Generator is already running");
      }

      if (state === GenStateCompleted) {
        if (method === "throw") {
          throw arg;
        }

        // Be forgiving, per 25.3.3.3.3 of the spec:
        // https://people.mozilla.org/~jorendorff/es6-draft.html#sec-generatorresume
        return doneResult();
      }

      context.method = method;
      context.arg = arg;

      while (true) {
        var delegate = context.delegate;
        if (delegate) {
          var delegateResult = maybeInvokeDelegate(delegate, context);
          if (delegateResult) {
            if (delegateResult === ContinueSentinel) continue;
            return delegateResult;
          }
        }

        if (context.method === "next") {
          // Setting context._sent for legacy support of Babel's
          // function.sent implementation.
          context.sent = context._sent = context.arg;

        } else if (context.method === "throw") {
          if (state === GenStateSuspendedStart) {
            state = GenStateCompleted;
            throw context.arg;
          }

          context.dispatchException(context.arg);

        } else if (context.method === "return") {
          context.abrupt("return", context.arg);
        }

        state = GenStateExecuting;

        var record = tryCatch(innerFn, self, context);
        if (record.type === "normal") {
          // If an exception is thrown from innerFn, we leave state ===
          // GenStateExecuting and loop back for another invocation.
          state = context.done
            ? GenStateCompleted
            : GenStateSuspendedYield;

          if (record.arg === ContinueSentinel) {
            continue;
          }

          return {
            value: record.arg,
            done: context.done
          };

        } else if (record.type === "throw") {
          state = GenStateCompleted;
          // Dispatch the exception by looping back around to the
          // context.dispatchException(context.arg) call above.
          context.method = "throw";
          context.arg = record.arg;
        }
      }
    };
  }

  // Call delegate.iterator[context.method](context.arg) and handle the
  // result, either by returning a { value, done } result from the
  // delegate iterator, or by modifying context.method and context.arg,
  // setting context.delegate to null, and returning the ContinueSentinel.
  function maybeInvokeDelegate(delegate, context) {
    var method = delegate.iterator[context.method];
    if (method === undefined) {
      // A .throw or .return when the delegate iterator has no .throw
      // method always terminates the yield* loop.
      context.delegate = null;

      if (context.method === "throw") {
        // Note: ["return"] must be used for ES3 parsing compatibility.
        if (delegate.iterator["return"]) {
          // If the delegate iterator has a return method, give it a
          // chance to clean up.
          context.method = "return";
          context.arg = undefined;
          maybeInvokeDelegate(delegate, context);

          if (context.method === "throw") {
            // If maybeInvokeDelegate(context) changed context.method from
            // "return" to "throw", let that override the TypeError below.
            return ContinueSentinel;
          }
        }

        context.method = "throw";
        context.arg = new TypeError(
          "The iterator does not provide a 'throw' method");
      }

      return ContinueSentinel;
    }

    var record = tryCatch(method, delegate.iterator, context.arg);

    if (record.type === "throw") {
      context.method = "throw";
      context.arg = record.arg;
      context.delegate = null;
      return ContinueSentinel;
    }

    var info = record.arg;

    if (! info) {
      context.method = "throw";
      context.arg = new TypeError("iterator result is not an object");
      context.delegate = null;
      return ContinueSentinel;
    }

    if (info.done) {
      // Assign the result of the finished delegate to the temporary
      // variable specified by delegate.resultName (see delegateYield).
      context[delegate.resultName] = info.value;

      // Resume execution at the desired location (see delegateYield).
      context.next = delegate.nextLoc;

      // If context.method was "throw" but the delegate handled the
      // exception, let the outer generator proceed normally. If
      // context.method was "next", forget context.arg since it has been
      // "consumed" by the delegate iterator. If context.method was
      // "return", allow the original .return call to continue in the
      // outer generator.
      if (context.method !== "return") {
        context.method = "next";
        context.arg = undefined;
      }

    } else {
      // Re-yield the result returned by the delegate method.
      return info;
    }

    // The delegate iterator is finished, so forget it and continue with
    // the outer generator.
    context.delegate = null;
    return ContinueSentinel;
  }

  // Define Generator.prototype.{next,throw,return} in terms of the
  // unified ._invoke helper method.
  defineIteratorMethods(Gp);

  define(Gp, toStringTagSymbol, "Generator");

  // A Generator should always return itself as the iterator object when the
  // @@iterator function is called on it. Some browsers' implementations of the
  // iterator prototype chain incorrectly implement this, causing the Generator
  // object to not be returned from this call. This ensures that doesn't happen.
  // See https://github.com/facebook/regenerator/issues/274 for more details.
  Gp[iteratorSymbol] = function() {
    return this;
  };

  Gp.toString = function() {
    return "[object Generator]";
  };

  function pushTryEntry(locs) {
    var entry = { tryLoc: locs[0] };

    if (1 in locs) {
      entry.catchLoc = locs[1];
    }

    if (2 in locs) {
      entry.finallyLoc = locs[2];
      entry.afterLoc = locs[3];
    }

    this.tryEntries.push(entry);
  }

  function resetTryEntry(entry) {
    var record = entry.completion || {};
    record.type = "normal";
    delete record.arg;
    entry.completion = record;
  }

  function Context(tryLocsList) {
    // The root entry object (effectively a try statement without a catch
    // or a finally block) gives us a place to store values thrown from
    // locations where there is no enclosing try statement.
    this.tryEntries = [{ tryLoc: "root" }];
    tryLocsList.forEach(pushTryEntry, this);
    this.reset(true);
  }

  exports.keys = function(object) {
    var keys = [];
    for (var key in object) {
      keys.push(key);
    }
    keys.reverse();

    // Rather than returning an object with a next method, we keep
    // things simple and return the next function itself.
    return function next() {
      while (keys.length) {
        var key = keys.pop();
        if (key in object) {
          next.value = key;
          next.done = false;
          return next;
        }
      }

      // To avoid creating an additional object, we just hang the .value
      // and .done properties off the next function object itself. This
      // also ensures that the minifier will not anonymize the function.
      next.done = true;
      return next;
    };
  };

  function values(iterable) {
    if (iterable) {
      var iteratorMethod = iterable[iteratorSymbol];
      if (iteratorMethod) {
        return iteratorMethod.call(iterable);
      }

      if (typeof iterable.next === "function") {
        return iterable;
      }

      if (!isNaN(iterable.length)) {
        var i = -1, next = function next() {
          while (++i < iterable.length) {
            if (hasOwn.call(iterable, i)) {
              next.value = iterable[i];
              next.done = false;
              return next;
            }
          }

          next.value = undefined;
          next.done = true;

          return next;
        };

        return next.next = next;
      }
    }

    // Return an iterator with no values.
    return { next: doneResult };
  }
  exports.values = values;

  function doneResult() {
    return { value: undefined, done: true };
  }

  Context.prototype = {
    constructor: Context,

    reset: function(skipTempReset) {
      this.prev = 0;
      this.next = 0;
      // Resetting context._sent for legacy support of Babel's
      // function.sent implementation.
      this.sent = this._sent = undefined;
      this.done = false;
      this.delegate = null;

      this.method = "next";
      this.arg = undefined;

      this.tryEntries.forEach(resetTryEntry);

      if (!skipTempReset) {
        for (var name in this) {
          // Not sure about the optimal order of these conditions:
          if (name.charAt(0) === "t" &&
              hasOwn.call(this, name) &&
              !isNaN(+name.slice(1))) {
            this[name] = undefined;
          }
        }
      }
    },

    stop: function() {
      this.done = true;

      var rootEntry = this.tryEntries[0];
      var rootRecord = rootEntry.completion;
      if (rootRecord.type === "throw") {
        throw rootRecord.arg;
      }

      return this.rval;
    },

    dispatchException: function(exception) {
      if (this.done) {
        throw exception;
      }

      var context = this;
      function handle(loc, caught) {
        record.type = "throw";
        record.arg = exception;
        context.next = loc;

        if (caught) {
          // If the dispatched exception was caught by a catch block,
          // then let that catch block handle the exception normally.
          context.method = "next";
          context.arg = undefined;
        }

        return !! caught;
      }

      for (var i = this.tryEntries.length - 1; i >= 0; --i) {
        var entry = this.tryEntries[i];
        var record = entry.completion;

        if (entry.tryLoc === "root") {
          // Exception thrown outside of any try block that could handle
          // it, so set the completion value of the entire function to
          // throw the exception.
          return handle("end");
        }

        if (entry.tryLoc <= this.prev) {
          var hasCatch = hasOwn.call(entry, "catchLoc");
          var hasFinally = hasOwn.call(entry, "finallyLoc");

          if (hasCatch && hasFinally) {
            if (this.prev < entry.catchLoc) {
              return handle(entry.catchLoc, true);
            } else if (this.prev < entry.finallyLoc) {
              return handle(entry.finallyLoc);
            }

          } else if (hasCatch) {
            if (this.prev < entry.catchLoc) {
              return handle(entry.catchLoc, true);
            }

          } else if (hasFinally) {
            if (this.prev < entry.finallyLoc) {
              return handle(entry.finallyLoc);
            }

          } else {
            throw new Error("try statement without catch or finally");
          }
        }
      }
    },

    abrupt: function(type, arg) {
      for (var i = this.tryEntries.length - 1; i >= 0; --i) {
        var entry = this.tryEntries[i];
        if (entry.tryLoc <= this.prev &&
            hasOwn.call(entry, "finallyLoc") &&
            this.prev < entry.finallyLoc) {
          var finallyEntry = entry;
          break;
        }
      }

      if (finallyEntry &&
          (type === "break" ||
           type === "continue") &&
          finallyEntry.tryLoc <= arg &&
          arg <= finallyEntry.finallyLoc) {
        // Ignore the finally entry if control is not jumping to a
        // location outside the try/catch block.
        finallyEntry = null;
      }

      var record = finallyEntry ? finallyEntry.completion : {};
      record.type = type;
      record.arg = arg;

      if (finallyEntry) {
        this.method = "next";
        this.next = finallyEntry.finallyLoc;
        return ContinueSentinel;
      }

      return this.complete(record);
    },

    complete: function(record, afterLoc) {
      if (record.type === "throw") {
        throw record.arg;
      }

      if (record.type === "break" ||
          record.type === "continue") {
        this.next = record.arg;
      } else if (record.type === "return") {
        this.rval = this.arg = record.arg;
        this.method = "return";
        this.next = "end";
      } else if (record.type === "normal" && afterLoc) {
        this.next = afterLoc;
      }

      return ContinueSentinel;
    },

    finish: function(finallyLoc) {
      for (var i = this.tryEntries.length - 1; i >= 0; --i) {
        var entry = this.tryEntries[i];
        if (entry.finallyLoc === finallyLoc) {
          this.complete(entry.completion, entry.afterLoc);
          resetTryEntry(entry);
          return ContinueSentinel;
        }
      }
    },

    "catch": function(tryLoc) {
      for (var i = this.tryEntries.length - 1; i >= 0; --i) {
        var entry = this.tryEntries[i];
        if (entry.tryLoc === tryLoc) {
          var record = entry.completion;
          if (record.type === "throw") {
            var thrown = record.arg;
            resetTryEntry(entry);
          }
          return thrown;
        }
      }

      // The context.catch method must only be called with a location
      // argument that corresponds to a known catch block.
      throw new Error("illegal catch attempt");
    },

    delegateYield: function(iterable, resultName, nextLoc) {
      this.delegate = {
        iterator: values(iterable),
        resultName: resultName,
        nextLoc: nextLoc
      };

      if (this.method === "next") {
        // Deliberately forget the last sent value so that we don't
        // accidentally pass it on to the delegate.
        this.arg = undefined;
      }

      return ContinueSentinel;
    }
  };

  // Regardless of whether this script is executing as a CommonJS module
  // or not, return the runtime object so that we can declare the variable
  // regeneratorRuntime in the outer scope, which allows this module to be
  // injected easily by `bin/regenerator --include-runtime script.js`.
  return exports;

}(
  // If this script is executing as a CommonJS module, use module.exports
  // as the regeneratorRuntime namespace. Otherwise create a new empty
  // object. Either way, the resulting object will be used to initialize
  // the regeneratorRuntime variable at the top of this file.
  typeof module === "object" ? module.exports : {}
));

try {
  regeneratorRuntime = runtime;
} catch (accidentalStrictMode) {
  // This module should not be running in strict mode, so the above
  // assignment should always work unless something is misconfigured. Just
  // in case runtime.js accidentally runs in strict mode, we can escape
  // strict mode using a global Function call. This could conceivably fail
  // if a Content Security Policy forbids using Function, but in that case
  // the proper solution is to fix the accidental strict mode problem. If
  // you've misconfigured your bundler to force strict mode and applied a
  // CSP to forbid Function, and you're not willing to fix either of those
  // problems, please detail your unique predicament in a GitHub issue.
  Function("r", "regeneratorRuntime = r")(runtime);
}

},{}],24:[function(require,module,exports){
// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// This file is borrowed from lynckia/licode with some modifications.
'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Base64 = void 0;

var Base64 = function () {
  var END_OF_INPUT = -1;
  var base64Str;
  var base64Count;
  var i;
  var base64Chars = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'];
  var reverseBase64Chars = [];

  for (i = 0; i < base64Chars.length; i = i + 1) {
    reverseBase64Chars[base64Chars[i]] = i;
  }

  var setBase64Str = function setBase64Str(str) {
    base64Str = str;
    base64Count = 0;
  };

  var readBase64 = function readBase64() {
    if (!base64Str) {
      return END_OF_INPUT;
    }

    if (base64Count >= base64Str.length) {
      return END_OF_INPUT;
    }

    var c = base64Str.charCodeAt(base64Count) & 0xff;
    base64Count = base64Count + 1;
    return c;
  };

  var encodeBase64 = function encodeBase64(str) {
    var result;
    var done;
    setBase64Str(str);
    result = '';
    var inBuffer = new Array(3);
    done = false;

    while (!done && (inBuffer[0] = readBase64()) !== END_OF_INPUT) {
      inBuffer[1] = readBase64();
      inBuffer[2] = readBase64();
      result = result + base64Chars[inBuffer[0] >> 2];

      if (inBuffer[1] !== END_OF_INPUT) {
        result = result + base64Chars[inBuffer[0] << 4 & 0x30 | inBuffer[1] >> 4];

        if (inBuffer[2] !== END_OF_INPUT) {
          result = result + base64Chars[inBuffer[1] << 2 & 0x3c | inBuffer[2] >> 6];
          result = result + base64Chars[inBuffer[2] & 0x3F];
        } else {
          result = result + base64Chars[inBuffer[1] << 2 & 0x3c];
          result = result + '=';
          done = true;
        }
      } else {
        result = result + base64Chars[inBuffer[0] << 4 & 0x30];
        result = result + '=';
        result = result + '=';
        done = true;
      }
    }

    return result;
  };

  var readReverseBase64 = function readReverseBase64() {
    if (!base64Str) {
      return END_OF_INPUT;
    }

    while (true) {
      // eslint-disable-line no-constant-condition
      if (base64Count >= base64Str.length) {
        return END_OF_INPUT;
      }

      var nextCharacter = base64Str.charAt(base64Count);
      base64Count = base64Count + 1;

      if (reverseBase64Chars[nextCharacter]) {
        return reverseBase64Chars[nextCharacter];
      }

      if (nextCharacter === 'A') {
        return 0;
      }
    }
  };

  var ntos = function ntos(n) {
    n = n.toString(16);

    if (n.length === 1) {
      n = '0' + n;
    }

    n = '%' + n;
    return unescape(n);
  };

  var decodeBase64 = function decodeBase64(str) {
    var result;
    var done;
    setBase64Str(str);
    result = '';
    var inBuffer = new Array(4);
    done = false;

    while (!done && (inBuffer[0] = readReverseBase64()) !== END_OF_INPUT && (inBuffer[1] = readReverseBase64()) !== END_OF_INPUT) {
      inBuffer[2] = readReverseBase64();
      inBuffer[3] = readReverseBase64();
      result = result + ntos(inBuffer[0] << 2 & 0xff | inBuffer[1] >> 4);

      if (inBuffer[2] !== END_OF_INPUT) {
        result += ntos(inBuffer[1] << 4 & 0xff | inBuffer[2] >> 2);

        if (inBuffer[3] !== END_OF_INPUT) {
          result = result + ntos(inBuffer[2] << 6 & 0xff | inBuffer[3]);
        } else {
          done = true;
        }
      } else {
        done = true;
      }
    }

    return result;
  };

  return {
    encodeBase64: encodeBase64,
    decodeBase64: decodeBase64
  };
}();

exports.Base64 = Base64;

},{}],25:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';
/**
 * @class AudioCodec
 * @memberOf Owt.Base
 * @classDesc Audio codec enumeration.
 * @hideconstructor
 */

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.VideoEncodingParameters = exports.VideoCodecParameters = exports.VideoCodec = exports.AudioEncodingParameters = exports.AudioCodecParameters = exports.AudioCodec = void 0;

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var AudioCodec = {
  PCMU: 'pcmu',
  PCMA: 'pcma',
  OPUS: 'opus',
  G722: 'g722',
  ISAC: 'iSAC',
  ILBC: 'iLBC',
  AAC: 'aac',
  AC3: 'ac3',
  NELLYMOSER: 'nellymoser'
};
/**
 * @class AudioCodecParameters
 * @memberOf Owt.Base
 * @classDesc Codec parameters for an audio track.
 * @hideconstructor
 */

exports.AudioCodec = AudioCodec;

var AudioCodecParameters = // eslint-disable-next-line require-jsdoc
function AudioCodecParameters(name, channelCount, clockRate) {
  (0, _classCallCheck2["default"])(this, AudioCodecParameters);

  /**
   * @member {string} name
   * @memberof Owt.Base.AudioCodecParameters
   * @instance
   * @desc Name of a codec. Please a value in Owt.Base.AudioCodec. However,
   * some functions do not support all the values in Owt.Base.AudioCodec.
   */
  this.name = name;
  /**
   * @member {?number} channelCount
   * @memberof Owt.Base.AudioCodecParameters
   * @instance
   * @desc Numbers of channels for an audio track.
   */

  this.channelCount = channelCount;
  /**
   * @member {?number} clockRate
   * @memberof Owt.Base.AudioCodecParameters
   * @instance
   * @desc The codec clock rate expressed in Hertz.
   */

  this.clockRate = clockRate;
};
/**
 * @class AudioEncodingParameters
 * @memberOf Owt.Base
 * @classDesc Encoding parameters for sending an audio track.
 * @hideconstructor
 */


exports.AudioCodecParameters = AudioCodecParameters;

var AudioEncodingParameters = // eslint-disable-next-line require-jsdoc
function AudioEncodingParameters(codec, maxBitrate) {
  (0, _classCallCheck2["default"])(this, AudioEncodingParameters);

  /**
   * @member {?Owt.Base.AudioCodecParameters} codec
   * @instance
   * @memberof Owt.Base.AudioEncodingParameters
   */
  this.codec = codec;
  /**
   * @member {?number} maxBitrate
   * @instance
   * @memberof Owt.Base.AudioEncodingParameters
   * @desc Max bitrate expressed in kbps.
   */

  this.maxBitrate = maxBitrate;
};
/**
 * @class VideoCodec
 * @memberOf Owt.Base
 * @classDesc Video codec enumeration.
 * @hideconstructor
 */


exports.AudioEncodingParameters = AudioEncodingParameters;
var VideoCodec = {
  VP8: 'vp8',
  VP9: 'vp9',
  H264: 'h264',
  H265: 'h265',
  AV1: 'av1',
  // Non-standard AV1, will be renamed to AV1.
  // https://bugs.chromium.org/p/webrtc/issues/detail?id=11042
  AV1X: 'av1x'
};
/**
 * @class VideoCodecParameters
 * @memberOf Owt.Base
 * @classDesc Codec parameters for a video track.
 * @hideconstructor
 */

exports.VideoCodec = VideoCodec;

var VideoCodecParameters = // eslint-disable-next-line require-jsdoc
function VideoCodecParameters(name, profile) {
  (0, _classCallCheck2["default"])(this, VideoCodecParameters);

  /**
   * @member {string} name
   * @memberof Owt.Base.VideoCodecParameters
   * @instance
   * @desc Name of a codec.Please a value in Owt.Base.AudioCodec.However,
     some functions do not support all the values in Owt.Base.AudioCodec.
   */
  this.name = name;
  /**
   * @member {?string} profile
   * @memberof Owt.Base.VideoCodecParameters
   * @instance
   * @desc The profile of a codec. Profile may not apply to all codecs.
   */

  this.profile = profile;
};
/**
 * @class VideoEncodingParameters
 * @memberOf Owt.Base
 * @classDesc Encoding parameters for sending a video track.
 * @hideconstructor
 */


exports.VideoCodecParameters = VideoCodecParameters;

var VideoEncodingParameters = // eslint-disable-next-line require-jsdoc
function VideoEncodingParameters(codec, maxBitrate) {
  (0, _classCallCheck2["default"])(this, VideoEncodingParameters);

  /**
   * @member {?Owt.Base.VideoCodecParameters} codec
   * @instance
   * @memberof Owt.Base.VideoEncodingParameters
   */
  this.codec = codec;
  /**
   * @member {?number} maxBitrate
   * @instance
   * @memberof Owt.Base.VideoEncodingParameters
   * @desc Max bitrate expressed in kbps.
   */

  this.maxBitrate = maxBitrate;
};

exports.VideoEncodingParameters = VideoEncodingParameters;

},{"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/interopRequireDefault":10}],26:[function(require,module,exports){
// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// This file is borrowed from lynckia/licode with some modifications.
'use strict';
/**
 * @class EventDispatcher
 * @classDesc A shim for EventTarget. Might be changed to EventTarget later.
 * @memberof Owt.Base
 * @hideconstructor
 */

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.MuteEvent = exports.ErrorEvent = exports.MessageEvent = exports.OwtEvent = exports.EventDispatcher = void 0;

var _inherits2 = _interopRequireDefault(require("@babel/runtime/helpers/inherits"));

var _possibleConstructorReturn2 = _interopRequireDefault(require("@babel/runtime/helpers/possibleConstructorReturn"));

var _getPrototypeOf2 = _interopRequireDefault(require("@babel/runtime/helpers/getPrototypeOf"));

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = (0, _getPrototypeOf2["default"])(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return (0, _possibleConstructorReturn2["default"])(this, result); }; }

function _isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

var EventDispatcher = function EventDispatcher() {
  // Private vars
  var spec = {};
  spec.dispatcher = {};
  spec.dispatcher.eventListeners = {};
  /**
   * @function addEventListener
   * @desc This function registers a callback function as a handler for the
   * corresponding event. It's shortened form is on(eventType, listener). See
   * the event description in the following table.
   * @instance
   * @memberof Owt.Base.EventDispatcher
   * @param {string} eventType Event string.
   * @param {function} listener Callback function.
   */

  this.addEventListener = function (eventType, listener) {
    if (spec.dispatcher.eventListeners[eventType] === undefined) {
      spec.dispatcher.eventListeners[eventType] = [];
    }

    spec.dispatcher.eventListeners[eventType].push(listener);
  };
  /**
   * @function removeEventListener
   * @desc This function removes a registered event listener.
   * @instance
   * @memberof Owt.Base.EventDispatcher
   * @param {string} eventType Event string.
   * @param {function} listener Callback function.
   */


  this.removeEventListener = function (eventType, listener) {
    if (!spec.dispatcher.eventListeners[eventType]) {
      return;
    }

    var index = spec.dispatcher.eventListeners[eventType].indexOf(listener);

    if (index !== -1) {
      spec.dispatcher.eventListeners[eventType].splice(index, 1);
    }
  };
  /**
   * @function clearEventListener
   * @desc This function removes all event listeners for one type.
   * @instance
   * @memberof Owt.Base.EventDispatcher
   * @param {string} eventType Event string.
   */


  this.clearEventListener = function (eventType) {
    spec.dispatcher.eventListeners[eventType] = [];
  }; // It dispatch a new event to the event listeners, based on the type
  // of event. All events are intended to be LicodeEvents.


  this.dispatchEvent = function (event) {
    if (!spec.dispatcher.eventListeners[event.type]) {
      return;
    }

    spec.dispatcher.eventListeners[event.type].map(function (listener) {
      listener(event);
    });
  };
};
/**
 * @class OwtEvent
 * @classDesc Class OwtEvent represents a generic Event in the library.
 * @memberof Owt.Base
 * @hideconstructor
 */


exports.EventDispatcher = EventDispatcher;

var OwtEvent = // eslint-disable-next-line require-jsdoc
function OwtEvent(type) {
  (0, _classCallCheck2["default"])(this, OwtEvent);
  this.type = type;
};
/**
 * @class MessageEvent
 * @classDesc Class MessageEvent represents a message Event in the library.
 * @memberof Owt.Base
 * @hideconstructor
 */


exports.OwtEvent = OwtEvent;

var MessageEvent = /*#__PURE__*/function (_OwtEvent) {
  (0, _inherits2["default"])(MessageEvent, _OwtEvent);

  var _super = _createSuper(MessageEvent);

  // eslint-disable-next-line require-jsdoc
  function MessageEvent(type, init) {
    var _this;

    (0, _classCallCheck2["default"])(this, MessageEvent);
    _this = _super.call(this, type);
    /**
     * @member {string} origin
     * @instance
     * @memberof Owt.Base.MessageEvent
     * @desc ID of the remote endpoint who published this stream.
     */

    _this.origin = init.origin;
    /**
     * @member {string} message
     * @instance
     * @memberof Owt.Base.MessageEvent
     */

    _this.message = init.message;
    /**
     * @member {string} to
     * @instance
     * @memberof Owt.Base.MessageEvent
     * @desc Values could be "all", "me" in conference mode, or undefined in
     * P2P mode.
     */

    _this.to = init.to;
    return _this;
  }

  return MessageEvent;
}(OwtEvent);
/**
 * @class ErrorEvent
 * @classDesc Class ErrorEvent represents an error Event in the library.
 * @memberof Owt.Base
 * @hideconstructor
 */


exports.MessageEvent = MessageEvent;

var ErrorEvent = /*#__PURE__*/function (_OwtEvent2) {
  (0, _inherits2["default"])(ErrorEvent, _OwtEvent2);

  var _super2 = _createSuper(ErrorEvent);

  // eslint-disable-next-line require-jsdoc
  function ErrorEvent(type, init) {
    var _this2;

    (0, _classCallCheck2["default"])(this, ErrorEvent);
    _this2 = _super2.call(this, type);
    /**
     * @member {Error} error
     * @instance
     * @memberof Owt.Base.ErrorEvent
     */

    _this2.error = init.error;
    return _this2;
  }

  return ErrorEvent;
}(OwtEvent);
/**
 * @class MuteEvent
 * @classDesc Class MuteEvent represents a mute or unmute event.
 * @memberof Owt.Base
 * @hideconstructor
 */


exports.ErrorEvent = ErrorEvent;

var MuteEvent = /*#__PURE__*/function (_OwtEvent3) {
  (0, _inherits2["default"])(MuteEvent, _OwtEvent3);

  var _super3 = _createSuper(MuteEvent);

  // eslint-disable-next-line require-jsdoc
  function MuteEvent(type, init) {
    var _this3;

    (0, _classCallCheck2["default"])(this, MuteEvent);
    _this3 = _super3.call(this, type);
    /**
     * @member {Owt.Base.TrackKind} kind
     * @instance
     * @memberof Owt.Base.MuteEvent
     */

    _this3.kind = init.kind;
    return _this3;
  }

  return MuteEvent;
}(OwtEvent);

exports.MuteEvent = MuteEvent;

},{"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/getPrototypeOf":8,"@babel/runtime/helpers/inherits":9,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/possibleConstructorReturn":16}],27:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});

var _mediastreamFactory = require("./mediastream-factory.js");

Object.keys(_mediastreamFactory).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  Object.defineProperty(exports, key, {
    enumerable: true,
    get: function get() {
      return _mediastreamFactory[key];
    }
  });
});

var _stream = require("./stream.js");

Object.keys(_stream).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  Object.defineProperty(exports, key, {
    enumerable: true,
    get: function get() {
      return _stream[key];
    }
  });
});

var _mediaformat = require("./mediaformat.js");

Object.keys(_mediaformat).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  Object.defineProperty(exports, key, {
    enumerable: true,
    get: function get() {
      return _mediaformat[key];
    }
  });
});

var _transport = require("./transport.js");

Object.keys(_transport).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  Object.defineProperty(exports, key, {
    enumerable: true,
    get: function get() {
      return _transport[key];
    }
  });
});

},{"./mediaformat.js":29,"./mediastream-factory.js":30,"./stream.js":33,"./transport.js":34}],28:[function(require,module,exports){
// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// This file is borrowed from lynckia/licode with some modifications.

/* global window */
'use strict';
/*
 * API to write logs based on traditional logging mechanisms: debug, trace,
 * info, warning, error
 */

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports["default"] = void 0;

var Logger = function () {
  var DEBUG = 0;
  var TRACE = 1;
  var INFO = 2;
  var WARNING = 3;
  var ERROR = 4;
  var NONE = 5;

  var noOp = function noOp() {}; // |that| is the object to be returned.


  var that = {
    DEBUG: DEBUG,
    TRACE: TRACE,
    INFO: INFO,
    WARNING: WARNING,
    ERROR: ERROR,
    NONE: NONE
  };

  that.log = function () {
    var _window$console;

    for (var _len = arguments.length, args = new Array(_len), _key = 0; _key < _len; _key++) {
      args[_key] = arguments[_key];
    }

    (_window$console = window.console).log.apply(_window$console, [new Date().toISOString()].concat(args));
  };

  var bindType = function bindType(type) {
    if (typeof window.console[type] === 'function') {
      return function () {
        var _window$console2;

        for (var _len2 = arguments.length, args = new Array(_len2), _key2 = 0; _key2 < _len2; _key2++) {
          args[_key2] = arguments[_key2];
        }

        (_window$console2 = window.console)[type].apply(_window$console2, [new Date().toISOString()].concat(args));
      };
    } else {
      return that.log;
    }
  };

  var setLogLevel = function setLogLevel(level) {
    if (level <= DEBUG) {
      that.debug = bindType('debug');
    } else {
      that.debug = noOp;
    }

    if (level <= TRACE) {
      that.trace = bindType('trace');
    } else {
      that.trace = noOp;
    }

    if (level <= INFO) {
      that.info = bindType('info');
    } else {
      that.info = noOp;
    }

    if (level <= WARNING) {
      that.warning = bindType('warn');
    } else {
      that.warning = noOp;
    }

    if (level <= ERROR) {
      that.error = bindType('error');
    } else {
      that.error = noOp;
    }
  };

  setLogLevel(DEBUG); // Default level is debug.

  that.setLogLevel = setLogLevel;
  return that;
}();

var _default = Logger;
exports["default"] = _default;

},{}],29:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';
/**
 * @class AudioSourceInfo
 * @classDesc Source info about an audio track. Values: 'mic', 'screen-cast',
 * 'file', 'mixed'.
 * @memberOf Owt.Base
 * @readonly
 * @enum {string}
 */

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Resolution = exports.TrackKind = exports.VideoSourceInfo = exports.AudioSourceInfo = void 0;

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var AudioSourceInfo = {
  MIC: 'mic',
  SCREENCAST: 'screen-cast',
  FILE: 'file',
  MIXED: 'mixed'
};
/**
 * @class VideoSourceInfo
 * @classDesc Source info about a video track. Values: 'camera', 'screen-cast',
 * 'file', 'mixed'.
 * @memberOf Owt.Base
 * @readonly
 * @enum {string}
 */

exports.AudioSourceInfo = AudioSourceInfo;
var VideoSourceInfo = {
  CAMERA: 'camera',
  SCREENCAST: 'screen-cast',
  FILE: 'file',
  MIXED: 'mixed'
};
/**
 * @class TrackKind
 * @classDesc Kind of a track. Values: 'audio' for audio track, 'video' for
 * video track, 'av' for both audio and video tracks.
 * @memberOf Owt.Base
 * @readonly
 * @enum {string}
 */

exports.VideoSourceInfo = VideoSourceInfo;
var TrackKind = {
  /**
   * Audio tracks.
   * @type string
   */
  AUDIO: 'audio',

  /**
   * Video tracks.
   * @type string
   */
  VIDEO: 'video',

  /**
   * Both audio and video tracks.
   * @type string
   */
  AUDIO_AND_VIDEO: 'av'
};
/**
 * @class Resolution
 * @memberOf Owt.Base
 * @classDesc The Resolution defines the size of a rectangle.
 * @constructor
 * @param {number} width
 * @param {number} height
 */

exports.TrackKind = TrackKind;

var Resolution = // eslint-disable-next-line require-jsdoc
function Resolution(width, height) {
  (0, _classCallCheck2["default"])(this, Resolution);

  /**
   * @member {number} width
   * @instance
   * @memberof Owt.Base.Resolution
   */
  this.width = width;
  /**
   * @member {number} height
   * @instance
   * @memberof Owt.Base.Resolution
   */

  this.height = height;
};

exports.Resolution = Resolution;

},{"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/interopRequireDefault":10}],30:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/* global Promise, navigator */
'use strict';

var _interopRequireWildcard = require("@babel/runtime/helpers/interopRequireWildcard");

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.MediaStreamFactory = exports.StreamConstraints = exports.VideoTrackConstraints = exports.AudioTrackConstraints = void 0;

var _createClass2 = _interopRequireDefault(require("@babel/runtime/helpers/createClass"));

var _typeof2 = _interopRequireDefault(require("@babel/runtime/helpers/typeof"));

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var utils = _interopRequireWildcard(require("./utils.js"));

var MediaFormatModule = _interopRequireWildcard(require("./mediaformat.js"));

/**
 * @class AudioTrackConstraints
 * @classDesc Constraints for creating an audio MediaStreamTrack.
 * @memberof Owt.Base
 * @constructor
 * @param {Owt.Base.AudioSourceInfo} source Source info of this audio track.
 */
var AudioTrackConstraints = // eslint-disable-next-line require-jsdoc
function AudioTrackConstraints(source) {
  (0, _classCallCheck2["default"])(this, AudioTrackConstraints);

  if (!Object.values(MediaFormatModule.AudioSourceInfo).some(function (v) {
    return v === source;
  })) {
    throw new TypeError('Invalid source.');
  }
  /**
   * @member {string} source
   * @memberof Owt.Base.AudioTrackConstraints
   * @desc Values could be "mic", "screen-cast", "file" or "mixed".
   * @instance
   */


  this.source = source;
  /**
   * @member {string} deviceId
   * @memberof Owt.Base.AudioTrackConstraints
   * @desc Do not provide deviceId if source is not "mic".
   * @instance
   * @see https://w3c.github.io/mediacapture-main/#def-constraint-deviceId
   */

  this.deviceId = undefined;
};
/**
 * @class VideoTrackConstraints
 * @classDesc Constraints for creating a video MediaStreamTrack.
 * @memberof Owt.Base
 * @constructor
 * @param {Owt.Base.VideoSourceInfo} source Source info of this video track.
 */


exports.AudioTrackConstraints = AudioTrackConstraints;

var VideoTrackConstraints = // eslint-disable-next-line require-jsdoc
function VideoTrackConstraints(source) {
  (0, _classCallCheck2["default"])(this, VideoTrackConstraints);

  if (!Object.values(MediaFormatModule.VideoSourceInfo).some(function (v) {
    return v === source;
  })) {
    throw new TypeError('Invalid source.');
  }
  /**
   * @member {string} source
   * @memberof Owt.Base.VideoTrackConstraints
   * @desc Values could be "camera", "screen-cast", "file" or "mixed".
   * @instance
   */


  this.source = source;
  /**
   * @member {string} deviceId
   * @memberof Owt.Base.VideoTrackConstraints
   * @desc Do not provide deviceId if source is not "camera".
   * @instance
   * @see https://w3c.github.io/mediacapture-main/#def-constraint-deviceId
   */

  this.deviceId = undefined;
  /**
   * @member {Owt.Base.Resolution} resolution
   * @memberof Owt.Base.VideoTrackConstraints
   * @instance
   */

  this.resolution = undefined;
  /**
   * @member {number} frameRate
   * @memberof Owt.Base.VideoTrackConstraints
   * @instance
   */

  this.frameRate = undefined;
};
/**
 * @class StreamConstraints
 * @classDesc Constraints for creating a MediaStream from screen mic and camera.
 * @memberof Owt.Base
 * @constructor
 * @param {?Owt.Base.AudioTrackConstraints} audioConstraints
 * @param {?Owt.Base.VideoTrackConstraints} videoConstraints
 */


exports.VideoTrackConstraints = VideoTrackConstraints;

var StreamConstraints = // eslint-disable-next-line require-jsdoc
function StreamConstraints() {
  var audioConstraints = arguments.length > 0 && arguments[0] !== undefined ? arguments[0] : false;
  var videoConstraints = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : false;
  (0, _classCallCheck2["default"])(this, StreamConstraints);

  /**
   * @member {Owt.Base.MediaStreamTrackDeviceConstraintsForAudio} audio
   * @memberof Owt.Base.MediaStreamDeviceConstraints
   * @instance
   */
  this.audio = audioConstraints;
  /**
   * @member {Owt.Base.MediaStreamTrackDeviceConstraintsForVideo} Video
   * @memberof Owt.Base.MediaStreamDeviceConstraints
   * @instance
   */

  this.video = videoConstraints;
}; // eslint-disable-next-line require-jsdoc


exports.StreamConstraints = StreamConstraints;

function isVideoConstrainsForScreenCast(constraints) {
  return (0, _typeof2["default"])(constraints.video) === 'object' && constraints.video.source === MediaFormatModule.VideoSourceInfo.SCREENCAST;
}
/**
 * @class MediaStreamFactory
 * @classDesc A factory to create MediaStream. You can also create MediaStream
 * by yourself.
 * @memberof Owt.Base
 */


var MediaStreamFactory = /*#__PURE__*/function () {
  function MediaStreamFactory() {
    (0, _classCallCheck2["default"])(this, MediaStreamFactory);
  }

  (0, _createClass2["default"])(MediaStreamFactory, null, [{
    key: "createMediaStream",

    /**
     * @function createMediaStream
     * @static
     * @desc Create a MediaStream with given constraints. If you want to create a
     * MediaStream for screen cast, please make sure both audio and video's source
     * are "screen-cast".
     * @memberof Owt.Base.MediaStreamFactory
     * @return {Promise<MediaStream, Error>} Return a promise that is resolved
     * when stream is successfully created, or rejected if one of the following
     * error happened:
     * - One or more parameters cannot be satisfied.
     * - Specified device is busy.
     * - Cannot obtain necessary permission or operation is canceled by user.
     * - Video source is screen cast, while audio source is not.
     * - Audio source is screen cast, while video source is disabled.
     * @param {Owt.Base.StreamConstraints} constraints
     */
    value: function createMediaStream(constraints) {
      if ((0, _typeof2["default"])(constraints) !== 'object' || !constraints.audio && !constraints.video) {
        return Promise.reject(new TypeError('Invalid constrains'));
      }

      if (!isVideoConstrainsForScreenCast(constraints) && (0, _typeof2["default"])(constraints.audio) === 'object' && constraints.audio.source === MediaFormatModule.AudioSourceInfo.SCREENCAST) {
        return Promise.reject(new TypeError('Cannot share screen without video.'));
      }

      if (isVideoConstrainsForScreenCast(constraints) && (0, _typeof2["default"])(constraints.audio) === 'object' && constraints.audio.source !== MediaFormatModule.AudioSourceInfo.SCREENCAST) {
        return Promise.reject(new TypeError('Cannot capture video from screen cast while capture audio from' + ' other source.'));
      } // Check and convert constraints.


      if (!constraints.audio && !constraints.video) {
        return Promise.reject(new TypeError('At least one of audio and video must be requested.'));
      }

      var mediaConstraints = Object.create({});

      if ((0, _typeof2["default"])(constraints.audio) === 'object' && constraints.audio.source === MediaFormatModule.AudioSourceInfo.MIC) {
        mediaConstraints.audio = Object.create({});

        if (utils.isEdge()) {
          mediaConstraints.audio.deviceId = constraints.audio.deviceId;
        } else {
          mediaConstraints.audio.deviceId = {
            exact: constraints.audio.deviceId
          };
        }
      } else {
        if (constraints.audio.source === MediaFormatModule.AudioSourceInfo.SCREENCAST) {
          mediaConstraints.audio = true;
        } else {
          mediaConstraints.audio = constraints.audio;
        }
      }

      if ((0, _typeof2["default"])(constraints.video) === 'object') {
        mediaConstraints.video = Object.create({});

        if (typeof constraints.video.frameRate === 'number') {
          mediaConstraints.video.frameRate = constraints.video.frameRate;
        }

        if (constraints.video.resolution && constraints.video.resolution.width && constraints.video.resolution.height) {
          if (constraints.video.source === MediaFormatModule.VideoSourceInfo.SCREENCAST) {
            mediaConstraints.video.width = constraints.video.resolution.width;
            mediaConstraints.video.height = constraints.video.resolution.height;
          } else {
            mediaConstraints.video.width = Object.create({});
            mediaConstraints.video.width.exact = constraints.video.resolution.width;
            mediaConstraints.video.height = Object.create({});
            mediaConstraints.video.height.exact = constraints.video.resolution.height;
          }
        }

        if (typeof constraints.video.deviceId === 'string') {
          mediaConstraints.video.deviceId = {
            exact: constraints.video.deviceId
          };
        }

        if (utils.isFirefox() && constraints.video.source === MediaFormatModule.VideoSourceInfo.SCREENCAST) {
          mediaConstraints.video.mediaSource = 'screen';
        }
      } else {
        mediaConstraints.video = constraints.video;
      }

      if (isVideoConstrainsForScreenCast(constraints)) {
        return navigator.mediaDevices.getDisplayMedia(mediaConstraints);
      } else {
        return navigator.mediaDevices.getUserMedia(mediaConstraints);
      }
    }
  }]);
  return MediaStreamFactory;
}();

exports.MediaStreamFactory = MediaStreamFactory;

},{"./mediaformat.js":29,"./utils.js":35,"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/createClass":7,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/interopRequireWildcard":11,"@babel/runtime/helpers/typeof":19}],31:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';

var _interopRequireWildcard = require("@babel/runtime/helpers/interopRequireWildcard");

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.PublishOptions = exports.Publication = exports.PublicationSettings = exports.VideoPublicationSettings = exports.AudioPublicationSettings = void 0;

var _assertThisInitialized2 = _interopRequireDefault(require("@babel/runtime/helpers/assertThisInitialized"));

var _inherits2 = _interopRequireDefault(require("@babel/runtime/helpers/inherits"));

var _possibleConstructorReturn2 = _interopRequireDefault(require("@babel/runtime/helpers/possibleConstructorReturn"));

var _getPrototypeOf2 = _interopRequireDefault(require("@babel/runtime/helpers/getPrototypeOf"));

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var Utils = _interopRequireWildcard(require("./utils.js"));

var _event = require("../base/event.js");

function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = (0, _getPrototypeOf2["default"])(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return (0, _possibleConstructorReturn2["default"])(this, result); }; }

function _isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

/**
 * @class AudioPublicationSettings
 * @memberOf Owt.Base
 * @classDesc The audio settings of a publication.
 * @hideconstructor
 */
var AudioPublicationSettings = // eslint-disable-next-line require-jsdoc
function AudioPublicationSettings(codec) {
  (0, _classCallCheck2["default"])(this, AudioPublicationSettings);

  /**
   * @member {?Owt.Base.AudioCodecParameters} codec
   * @instance
   * @memberof Owt.Base.AudioPublicationSettings
   */
  this.codec = codec;
};
/**
 * @class VideoPublicationSettings
 * @memberOf Owt.Base
 * @classDesc The video settings of a publication.
 * @hideconstructor
 */


exports.AudioPublicationSettings = AudioPublicationSettings;

var VideoPublicationSettings = // eslint-disable-next-line require-jsdoc
function VideoPublicationSettings(codec, resolution, frameRate, bitrate, keyFrameInterval, rid) {
  (0, _classCallCheck2["default"])(this, VideoPublicationSettings);

  /**
   * @member {?Owt.Base.VideoCodecParameters} codec
   * @instance
   * @memberof Owt.Base.VideoPublicationSettings
   */
  this.codec = codec,
  /**
   * @member {?Owt.Base.Resolution} resolution
   * @instance
   * @memberof Owt.Base.VideoPublicationSettings
   */
  this.resolution = resolution;
  /**
   * @member {?number} frameRates
   * @instance
   * @classDesc Frames per second.
   * @memberof Owt.Base.VideoPublicationSettings
   */

  this.frameRate = frameRate;
  /**
   * @member {?number} bitrate
   * @instance
   * @memberof Owt.Base.VideoPublicationSettings
   */

  this.bitrate = bitrate;
  /**
   * @member {?number} keyFrameIntervals
   * @instance
   * @classDesc The time interval between key frames. Unit: second.
   * @memberof Owt.Base.VideoPublicationSettings
   */

  this.keyFrameInterval = keyFrameInterval;
  /**
   * @member {?number} rid
   * @instance
   * @classDesc Restriction identifier to identify the RTP Streams within an RTP session.
   * @memberof Owt.Base.VideoPublicationSettings
   */

  this.rid = rid;
};
/**
 * @class PublicationSettings
 * @memberOf Owt.Base
 * @classDesc The settings of a publication.
 * @hideconstructor
 */


exports.VideoPublicationSettings = VideoPublicationSettings;

var PublicationSettings = // eslint-disable-next-line require-jsdoc
function PublicationSettings(audio, video) {
  (0, _classCallCheck2["default"])(this, PublicationSettings);

  /**
   * @member {Owt.Base.AudioPublicationSettings[]} audio
   * @instance
   * @memberof Owt.Base.PublicationSettings
   */
  this.audio = audio;
  /**
   * @member {Owt.Base.VideoPublicationSettings[]} video
   * @instance
   * @memberof Owt.Base.PublicationSettings
   */

  this.video = video;
};
/**
 * @class Publication
 * @extends Owt.Base.EventDispatcher
 * @memberOf Owt.Base
 * @classDesc Publication represents a sender for publishing a stream. It
 * handles the actions on a LocalStream published to a conference.
 *
 * Events:
 *
 * | Event Name      | Argument Type    | Fired when       |
 * | ----------------| ---------------- | ---------------- |
 * | ended           | Event            | Publication is ended. |
 * | error           | ErrorEvent       | An error occurred on the publication. |
 * | mute            | MuteEvent        | Publication is muted. Client stopped sending audio and/or video data to remote endpoint. |
 * | unmute          | MuteEvent        | Publication is unmuted. Client continued sending audio and/or video data to remote endpoint. |
 *
 * `ended` event may not be fired on Safari after calling `Publication.stop()`.
 *
 * @hideconstructor
 */


exports.PublicationSettings = PublicationSettings;

var Publication = /*#__PURE__*/function (_EventDispatcher) {
  (0, _inherits2["default"])(Publication, _EventDispatcher);

  var _super = _createSuper(Publication);

  // eslint-disable-next-line require-jsdoc
  function Publication(id, transport, stop, getStats, mute, unmute) {
    var _this;

    (0, _classCallCheck2["default"])(this, Publication);
    _this = _super.call(this);
    /**
     * @member {string} id
     * @instance
     * @memberof Owt.Base.Publication
     */

    Object.defineProperty((0, _assertThisInitialized2["default"])(_this), 'id', {
      configurable: false,
      writable: false,
      value: id ? id : Utils.createUuid()
    });
    /**
     * @member {Owt.Base.TransportSettings} transport
     * @instance
     * @readonly
     * @desc Transport settings for the publication.
     * @memberof Owt.Base.Publication
     */

    Object.defineProperty((0, _assertThisInitialized2["default"])(_this), 'transport', {
      configurable: false,
      writable: false,
      value: transport
    });
    /**
     * @function stop
     * @instance
     * @desc Stop certain publication. Once a subscription is stopped, it cannot
     * be recovered.
     * @memberof Owt.Base.Publication
     * @returns {undefined}
     */

    _this.stop = stop;
    /**
     * @function getStats
     * @instance
     * @desc Get stats of underlying PeerConnection.
     * @memberof Owt.Base.Publication
     * @returns {Promise<RTCStatsReport, Error>}
     */

    _this.getStats = getStats;
    /**
     * @function mute
     * @instance
     * @desc Stop sending data to remote endpoint.
     * @memberof Owt.Base.Publication
     * @param {Owt.Base.TrackKind } kind Kind of tracks to be muted.
     * @returns {Promise<undefined, Error>}
     */

    _this.mute = mute;
    /**
     * @function unmute
     * @instance
     * @desc Continue sending data to remote endpoint.
     * @memberof Owt.Base.Publication
     * @param {Owt.Base.TrackKind } kind Kind of tracks to be unmuted.
     * @returns {Promise<undefined, Error>}
     */

    _this.unmute = unmute;
    return _this;
  }

  return Publication;
}(_event.EventDispatcher);
/**
 * @class PublishOptions
 * @memberOf Owt.Base
 * @classDesc PublishOptions defines options for publishing a
 * Owt.Base.LocalStream.
 */


exports.Publication = Publication;

var PublishOptions = // eslint-disable-next-line require-jsdoc
function PublishOptions(audio, video, transport) {
  (0, _classCallCheck2["default"])(this, PublishOptions);

  /**
   * @member {?Array<Owt.Base.AudioEncodingParameters> | ?Array<RTCRtpEncodingParameters>} audio
   * @instance
   * @memberof Owt.Base.PublishOptions
   * @desc Parameters for audio RtpSender. Publishing with RTCRtpEncodingParameters is an experimental feature. It is subject to change.
   */
  this.audio = audio;
  /**
   * @member {?Array<Owt.Base.VideoEncodingParameters> | ?Array<RTCRtpEncodingParameters>} video
   * @instance
   * @memberof Owt.Base.PublishOptions
   * @desc Parameters for video RtpSender. Publishing with RTCRtpEncodingParameters is an experimental feature. It is subject to change.
   */

  this.video = video;
  /**
   * @member {?Owt.Base.TransportConstraints} transport
   * @instance
   * @memberof Owt.Base.PublishOptions
   */

  this.transport = transport;
};

exports.PublishOptions = PublishOptions;

},{"../base/event.js":26,"./utils.js":35,"@babel/runtime/helpers/assertThisInitialized":3,"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/getPrototypeOf":8,"@babel/runtime/helpers/inherits":9,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/interopRequireWildcard":11,"@babel/runtime/helpers/possibleConstructorReturn":16}],32:[function(require,module,exports){
"use strict";

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.reorderCodecs = reorderCodecs;
exports.addLegacySimulcast = addLegacySimulcast;
exports.setMaxBitrate = setMaxBitrate;

var _logger = _interopRequireDefault(require("./logger.js"));

function _createForOfIteratorHelper(o, allowArrayLike) { var it; if (typeof Symbol === "undefined" || o[Symbol.iterator] == null) { if (Array.isArray(o) || (it = _unsupportedIterableToArray(o)) || allowArrayLike && o && typeof o.length === "number") { if (it) o = it; var i = 0; var F = function F() {}; return { s: F, n: function n() { if (i >= o.length) return { done: true }; return { done: false, value: o[i++] }; }, e: function e(_e) { throw _e; }, f: F }; } throw new TypeError("Invalid attempt to iterate non-iterable instance.\nIn order to be iterable, non-array objects must have a [Symbol.iterator]() method."); } var normalCompletion = true, didErr = false, err; return { s: function s() { it = o[Symbol.iterator](); }, n: function n() { var step = it.next(); normalCompletion = step.done; return step; }, e: function e(_e2) { didErr = true; err = _e2; }, f: function f() { try { if (!normalCompletion && it["return"] != null) it["return"](); } finally { if (didErr) throw err; } } }; }

function _unsupportedIterableToArray(o, minLen) { if (!o) return; if (typeof o === "string") return _arrayLikeToArray(o, minLen); var n = Object.prototype.toString.call(o).slice(8, -1); if (n === "Object" && o.constructor) n = o.constructor.name; if (n === "Map" || n === "Set") return Array.from(o); if (n === "Arguments" || /^(?:Ui|I)nt(?:8|16|32)(?:Clamped)?Array$/.test(n)) return _arrayLikeToArray(o, minLen); }

function _arrayLikeToArray(arr, len) { if (len == null || len > arr.length) len = arr.length; for (var i = 0, arr2 = new Array(len); i < len; i++) { arr2[i] = arr[i]; } return arr2; }

'use strict';

function mergeConstraints(cons1, cons2) {
  if (!cons1 || !cons2) {
    return cons1 || cons2;
  }

  var merged = cons1;

  for (var key in cons2) {
    merged[key] = cons2[key];
  }

  return merged;
}

function iceCandidateType(candidateStr) {
  return candidateStr.split(' ')[7];
} // Turns the local type preference into a human-readable string.
// Note that this mapping is browser-specific.


function formatTypePreference(pref) {
  if (adapter.browserDetails.browser === 'chrome') {
    switch (pref) {
      case 0:
        return 'TURN/TLS';

      case 1:
        return 'TURN/TCP';

      case 2:
        return 'TURN/UDP';

      default:
        break;
    }
  } else if (adapter.browserDetails.browser === 'firefox') {
    switch (pref) {
      case 0:
        return 'TURN/TCP';

      case 5:
        return 'TURN/UDP';

      default:
        break;
    }
  }

  return '';
}

function maybeSetOpusOptions(sdp, params) {
  // Set Opus in Stereo, if stereo is true, unset it, if stereo is false, and
  // do nothing if otherwise.
  if (params.opusStereo === 'true') {
    sdp = setCodecParam(sdp, 'opus/48000', 'stereo', '1');
  } else if (params.opusStereo === 'false') {
    sdp = removeCodecParam(sdp, 'opus/48000', 'stereo');
  } // Set Opus FEC, if opusfec is true, unset it, if opusfec is false, and
  // do nothing if otherwise.


  if (params.opusFec === 'true') {
    sdp = setCodecParam(sdp, 'opus/48000', 'useinbandfec', '1');
  } else if (params.opusFec === 'false') {
    sdp = removeCodecParam(sdp, 'opus/48000', 'useinbandfec');
  } // Set Opus DTX, if opusdtx is true, unset it, if opusdtx is false, and
  // do nothing if otherwise.


  if (params.opusDtx === 'true') {
    sdp = setCodecParam(sdp, 'opus/48000', 'usedtx', '1');
  } else if (params.opusDtx === 'false') {
    sdp = removeCodecParam(sdp, 'opus/48000', 'usedtx');
  } // Set Opus maxplaybackrate, if requested.


  if (params.opusMaxPbr) {
    sdp = setCodecParam(sdp, 'opus/48000', 'maxplaybackrate', params.opusMaxPbr);
  }

  return sdp;
}

function maybeSetAudioSendBitRate(sdp, params) {
  if (!params.audioSendBitrate) {
    return sdp;
  }

  _logger["default"].debug('Prefer audio send bitrate: ' + params.audioSendBitrate);

  return preferBitRate(sdp, params.audioSendBitrate, 'audio');
}

function maybeSetAudioReceiveBitRate(sdp, params) {
  if (!params.audioRecvBitrate) {
    return sdp;
  }

  _logger["default"].debug('Prefer audio receive bitrate: ' + params.audioRecvBitrate);

  return preferBitRate(sdp, params.audioRecvBitrate, 'audio');
}

function maybeSetVideoSendBitRate(sdp, params) {
  if (!params.videoSendBitrate) {
    return sdp;
  }

  _logger["default"].debug('Prefer video send bitrate: ' + params.videoSendBitrate);

  return preferBitRate(sdp, params.videoSendBitrate, 'video');
}

function maybeSetVideoReceiveBitRate(sdp, params) {
  if (!params.videoRecvBitrate) {
    return sdp;
  }

  _logger["default"].debug('Prefer video receive bitrate: ' + params.videoRecvBitrate);

  return preferBitRate(sdp, params.videoRecvBitrate, 'video');
} // Add a b=AS:bitrate line to the m=mediaType section.


function preferBitRate(sdp, bitrate, mediaType) {
  var sdpLines = sdp.split('\r\n'); // Find m line for the given mediaType.

  var mLineIndex = findLine(sdpLines, 'm=', mediaType);

  if (mLineIndex === null) {
    _logger["default"].debug('Failed to add bandwidth line to sdp, as no m-line found');

    return sdp;
  } // Find next m-line if any.


  var nextMLineIndex = findLineInRange(sdpLines, mLineIndex + 1, -1, 'm=');

  if (nextMLineIndex === null) {
    nextMLineIndex = sdpLines.length;
  } // Find c-line corresponding to the m-line.


  var cLineIndex = findLineInRange(sdpLines, mLineIndex + 1, nextMLineIndex, 'c=');

  if (cLineIndex === null) {
    _logger["default"].debug('Failed to add bandwidth line to sdp, as no c-line found');

    return sdp;
  } // Check if bandwidth line already exists between c-line and next m-line.


  var bLineIndex = findLineInRange(sdpLines, cLineIndex + 1, nextMLineIndex, 'b=AS');

  if (bLineIndex) {
    sdpLines.splice(bLineIndex, 1);
  } // Create the b (bandwidth) sdp line.


  var bwLine = 'b=AS:' + bitrate; // As per RFC 4566, the b line should follow after c-line.

  sdpLines.splice(cLineIndex + 1, 0, bwLine);
  sdp = sdpLines.join('\r\n');
  return sdp;
} // Add an a=fmtp: x-google-min-bitrate=kbps line, if videoSendInitialBitrate
// is specified. We'll also add a x-google-min-bitrate value, since the max
// must be >= the min.


function maybeSetVideoSendInitialBitRate(sdp, params) {
  var initialBitrate = parseInt(params.videoSendInitialBitrate);

  if (!initialBitrate) {
    return sdp;
  } // Validate the initial bitrate value.


  var maxBitrate = parseInt(initialBitrate);
  var bitrate = parseInt(params.videoSendBitrate);

  if (bitrate) {
    if (initialBitrate > bitrate) {
      _logger["default"].debug('Clamping initial bitrate to max bitrate of ' + bitrate + ' kbps.');

      initialBitrate = bitrate;
      params.videoSendInitialBitrate = initialBitrate;
    }

    maxBitrate = bitrate;
  }

  var sdpLines = sdp.split('\r\n'); // Search for m line.

  var mLineIndex = findLine(sdpLines, 'm=', 'video');

  if (mLineIndex === null) {
    _logger["default"].debug('Failed to find video m-line');

    return sdp;
  } // Figure out the first codec payload type on the m=video SDP line.


  var videoMLine = sdpLines[mLineIndex];
  var pattern = new RegExp('m=video\\s\\d+\\s[A-Z/]+\\s');
  var sendPayloadType = videoMLine.split(pattern)[1].split(' ')[0];
  var fmtpLine = sdpLines[findLine(sdpLines, 'a=rtpmap', sendPayloadType)];
  var codecName = fmtpLine.split('a=rtpmap:' + sendPayloadType)[1].split('/')[0]; // Use codec from params if specified via URL param, otherwise use from SDP.

  var codec = params.videoSendCodec || codecName;
  sdp = setCodecParam(sdp, codec, 'x-google-min-bitrate', params.videoSendInitialBitrate.toString());
  sdp = setCodecParam(sdp, codec, 'x-google-max-bitrate', maxBitrate.toString());
  return sdp;
}

function removePayloadTypeFromMline(mLine, payloadType) {
  mLine = mLine.split(' ');

  for (var i = 0; i < mLine.length; ++i) {
    if (mLine[i] === payloadType.toString()) {
      mLine.splice(i, 1);
    }
  }

  return mLine.join(' ');
}

function removeCodecByName(sdpLines, codec) {
  var index = findLine(sdpLines, 'a=rtpmap', codec);

  if (index === null) {
    return sdpLines;
  }

  var payloadType = getCodecPayloadTypeFromLine(sdpLines[index]);
  sdpLines.splice(index, 1); // Search for the video m= line and remove the codec.

  var mLineIndex = findLine(sdpLines, 'm=', 'video');

  if (mLineIndex === null) {
    return sdpLines;
  }

  sdpLines[mLineIndex] = removePayloadTypeFromMline(sdpLines[mLineIndex], payloadType);
  return sdpLines;
}

function removeCodecByPayloadType(sdpLines, payloadType) {
  var index = findLine(sdpLines, 'a=rtpmap', payloadType.toString());

  if (index === null) {
    return sdpLines;
  }

  sdpLines.splice(index, 1); // Search for the video m= line and remove the codec.

  var mLineIndex = findLine(sdpLines, 'm=', 'video');

  if (mLineIndex === null) {
    return sdpLines;
  }

  sdpLines[mLineIndex] = removePayloadTypeFromMline(sdpLines[mLineIndex], payloadType);
  return sdpLines;
}

function maybeRemoveVideoFec(sdp, params) {
  if (params.videoFec !== 'false') {
    return sdp;
  }

  var sdpLines = sdp.split('\r\n');
  var index = findLine(sdpLines, 'a=rtpmap', 'red');

  if (index === null) {
    return sdp;
  }

  var redPayloadType = getCodecPayloadTypeFromLine(sdpLines[index]);
  sdpLines = removeCodecByPayloadType(sdpLines, redPayloadType);
  sdpLines = removeCodecByName(sdpLines, 'ulpfec'); // Remove fmtp lines associated with red codec.

  index = findLine(sdpLines, 'a=fmtp', redPayloadType.toString());

  if (index === null) {
    return sdp;
  }

  var fmtpLine = parseFmtpLine(sdpLines[index]);
  var rtxPayloadType = fmtpLine.pt;

  if (rtxPayloadType === null) {
    return sdp;
  }

  sdpLines.splice(index, 1);
  sdpLines = removeCodecByPayloadType(sdpLines, rtxPayloadType);
  return sdpLines.join('\r\n');
} // Promotes |audioSendCodec| to be the first in the m=audio line, if set.


function maybePreferAudioSendCodec(sdp, params) {
  return maybePreferCodec(sdp, 'audio', 'send', params.audioSendCodec);
} // Promotes |audioRecvCodec| to be the first in the m=audio line, if set.


function maybePreferAudioReceiveCodec(sdp, params) {
  return maybePreferCodec(sdp, 'audio', 'receive', params.audioRecvCodec);
} // Promotes |videoSendCodec| to be the first in the m=audio line, if set.


function maybePreferVideoSendCodec(sdp, params) {
  return maybePreferCodec(sdp, 'video', 'send', params.videoSendCodec);
} // Promotes |videoRecvCodec| to be the first in the m=audio line, if set.


function maybePreferVideoReceiveCodec(sdp, params) {
  return maybePreferCodec(sdp, 'video', 'receive', params.videoRecvCodec);
} // Sets |codec| as the default |type| codec if it's present.
// The format of |codec| is 'NAME/RATE', e.g. 'opus/48000'.


function maybePreferCodec(sdp, type, dir, codec) {
  var str = type + ' ' + dir + ' codec';

  if (!codec) {
    _logger["default"].debug('No preference on ' + str + '.');

    return sdp;
  }

  _logger["default"].debug('Prefer ' + str + ': ' + codec);

  var sdpLines = sdp.split('\r\n'); // Search for m line.

  var mLineIndex = findLine(sdpLines, 'm=', type);

  if (mLineIndex === null) {
    return sdp;
  } // If the codec is available, set it as the default in m line.


  var payload = null;

  for (var i = 0; i < sdpLines.length; i++) {
    var index = findLineInRange(sdpLines, i, -1, 'a=rtpmap', codec);

    if (index !== null) {
      payload = getCodecPayloadTypeFromLine(sdpLines[index]);

      if (payload) {
        sdpLines[mLineIndex] = setDefaultCodec(sdpLines[mLineIndex], payload);
      }
    }
  }

  sdp = sdpLines.join('\r\n');
  return sdp;
} // Set fmtp param to specific codec in SDP. If param does not exists, add it.


function setCodecParam(sdp, codec, param, value, mid) {
  var sdpLines = sdp.split('\r\n');
  var headLines = null;
  var tailLines = null;

  if (typeof mid === 'string') {
    var midRange = findMLineRangeWithMID(sdpLines, mid);

    if (midRange) {
      var start = midRange.start,
          end = midRange.end;
      headLines = sdpLines.slice(0, start);
      tailLines = sdpLines.slice(end);
      sdpLines = sdpLines.slice(start, end);
    }
  } // SDPs sent from MCU use \n as line break.


  if (sdpLines.length <= 1) {
    sdpLines = sdp.split('\n');
  }

  var fmtpLineIndex = findFmtpLine(sdpLines, codec);
  var fmtpObj = {};

  if (fmtpLineIndex === null) {
    var index = findLine(sdpLines, 'a=rtpmap', codec);

    if (index === null) {
      return sdp;
    }

    var payload = getCodecPayloadTypeFromLine(sdpLines[index]);
    fmtpObj.pt = payload.toString();
    fmtpObj.params = {};
    fmtpObj.params[param] = value;
    sdpLines.splice(index + 1, 0, writeFmtpLine(fmtpObj));
  } else {
    fmtpObj = parseFmtpLine(sdpLines[fmtpLineIndex]);
    fmtpObj.params[param] = value;
    sdpLines[fmtpLineIndex] = writeFmtpLine(fmtpObj);
  }

  if (headLines) {
    sdpLines = headLines.concat(sdpLines).concat(tailLines);
  }

  sdp = sdpLines.join('\r\n');
  return sdp;
} // Remove fmtp param if it exists.


function removeCodecParam(sdp, codec, param) {
  var sdpLines = sdp.split('\r\n');
  var fmtpLineIndex = findFmtpLine(sdpLines, codec);

  if (fmtpLineIndex === null) {
    return sdp;
  }

  var map = parseFmtpLine(sdpLines[fmtpLineIndex]);
  delete map.params[param];
  var newLine = writeFmtpLine(map);

  if (newLine === null) {
    sdpLines.splice(fmtpLineIndex, 1);
  } else {
    sdpLines[fmtpLineIndex] = newLine;
  }

  sdp = sdpLines.join('\r\n');
  return sdp;
} // Split an fmtp line into an object including 'pt' and 'params'.


function parseFmtpLine(fmtpLine) {
  var fmtpObj = {};
  var spacePos = fmtpLine.indexOf(' ');
  var keyValues = fmtpLine.substring(spacePos + 1).split(';');
  var pattern = new RegExp('a=fmtp:(\\d+)');
  var result = fmtpLine.match(pattern);

  if (result && result.length === 2) {
    fmtpObj.pt = result[1];
  } else {
    return null;
  }

  var params = {};

  for (var i = 0; i < keyValues.length; ++i) {
    var pair = keyValues[i].split('=');

    if (pair.length === 2) {
      params[pair[0]] = pair[1];
    }
  }

  fmtpObj.params = params;
  return fmtpObj;
} // Generate an fmtp line from an object including 'pt' and 'params'.


function writeFmtpLine(fmtpObj) {
  if (!fmtpObj.hasOwnProperty('pt') || !fmtpObj.hasOwnProperty('params')) {
    return null;
  }

  var pt = fmtpObj.pt;
  var params = fmtpObj.params;
  var keyValues = [];
  var i = 0;

  for (var key in params) {
    keyValues[i] = key + '=' + params[key];
    ++i;
  }

  if (i === 0) {
    return null;
  }

  return 'a=fmtp:' + pt.toString() + ' ' + keyValues.join(';');
} // Find fmtp attribute for |codec| in |sdpLines|.


function findFmtpLine(sdpLines, codec) {
  // Find payload of codec.
  var payload = getCodecPayloadType(sdpLines, codec); // Find the payload in fmtp line.

  return payload ? findLine(sdpLines, 'a=fmtp:' + payload.toString()) : null;
} // Find the line in sdpLines that starts with |prefix|, and, if specified,
// contains |substr| (case-insensitive search).


function findLine(sdpLines, prefix, substr) {
  return findLineInRange(sdpLines, 0, -1, prefix, substr);
} // Find the line in sdpLines[startLine...endLine - 1] that starts with |prefix|
// and, if specified, contains |substr| (case-insensitive search).


function findLineInRange(sdpLines, startLine, endLine, prefix, substr) {
  var realEndLine = endLine !== -1 ? endLine : sdpLines.length;

  for (var i = startLine; i < realEndLine; ++i) {
    if (sdpLines[i].indexOf(prefix) === 0) {
      if (!substr || sdpLines[i].toLowerCase().indexOf(substr.toLowerCase()) !== -1) {
        return i;
      }
    }
  }

  return null;
} // Gets the codec payload type from sdp lines.


function getCodecPayloadType(sdpLines, codec) {
  var index = findLine(sdpLines, 'a=rtpmap', codec);
  return index ? getCodecPayloadTypeFromLine(sdpLines[index]) : null;
} // Gets the codec payload type from an a=rtpmap:X line.


function getCodecPayloadTypeFromLine(sdpLine) {
  var pattern = new RegExp('a=rtpmap:(\\d+) [a-zA-Z0-9-]+\\/\\d+');
  var result = sdpLine.match(pattern);
  return result && result.length === 2 ? result[1] : null;
} // Returns a new m= line with the specified codec as the first one.


function setDefaultCodec(mLine, payload) {
  var elements = mLine.split(' '); // Just copy the first three parameters; codec order starts on fourth.

  var newLine = elements.slice(0, 3); // Put target payload first and copy in the rest.

  newLine.push(payload);

  for (var i = 3; i < elements.length; i++) {
    if (elements[i] !== payload) {
      newLine.push(elements[i]);
    }
  }

  return newLine.join(' ');
}
/* Below are newly added functions */
// Following codecs will not be removed from SDP event they are not in the
// user-specified codec list.


var audioCodecAllowList = ['CN', 'telephone-event'];
var videoCodecAllowList = ['red', 'ulpfec', 'flexfec']; // Returns a new m= line with the specified codec order.

function setCodecOrder(mLine, payloads) {
  var elements = mLine.split(' '); // Just copy the first three parameters; codec order starts on fourth.

  var newLine = elements.slice(0, 3); // Concat payload types.

  newLine = newLine.concat(payloads);
  return newLine.join(' ');
} // Append RTX payloads for existing payloads.


function appendRtxPayloads(sdpLines, payloads) {
  var _iterator = _createForOfIteratorHelper(payloads),
      _step;

  try {
    for (_iterator.s(); !(_step = _iterator.n()).done;) {
      var payload = _step.value;
      var index = findLine(sdpLines, 'a=fmtp', 'apt=' + payload);

      if (index !== null) {
        var fmtpLine = parseFmtpLine(sdpLines[index]);
        payloads.push(fmtpLine.pt);
      }
    }
  } catch (err) {
    _iterator.e(err);
  } finally {
    _iterator.f();
  }

  return payloads;
} // Remove a codec with all its associated a lines.


function removeCodecFramALine(sdpLines, payload) {
  var pattern = new RegExp('a=(rtpmap|rtcp-fb|fmtp):' + payload + '\\s');

  for (var i = sdpLines.length - 1; i > 0; i--) {
    if (sdpLines[i].match(pattern)) {
      sdpLines.splice(i, 1);
    }
  }

  return sdpLines;
} // Find m-line and next m-line with give mid, return { start, end }.


function findMLineRangeWithMID(sdpLines, mid) {
  var midLine = 'a=mid:' + mid;
  var midIndex = findLine(sdpLines, midLine); // Compare the whole line since findLine only compares prefix

  while (midIndex >= 0 && sdpLines[midIndex] !== midLine) {
    midIndex = findLineInRange(sdpLines, midIndex, -1, midLine);
  }

  if (midIndex >= 0) {
    // Found matched a=mid line
    var nextMLineIndex = findLineInRange(sdpLines, midIndex, -1, 'm=') || -1;
    var mLineIndex = -1;

    for (var i = midIndex; i >= 0; i--) {
      if (sdpLines[i].indexOf('m=') >= 0) {
        mLineIndex = i;
        break;
      }
    }

    if (mLineIndex >= 0) {
      return {
        start: mLineIndex,
        end: nextMLineIndex
      };
    }
  }

  return null;
} // Reorder codecs in m-line according the order of |codecs|. Remove codecs from
// m-line if it is not present in |codecs|
// Applied on specific m-line if mid is presented
// The format of |codec| is 'NAME/RATE', e.g. 'opus/48000'.


function reorderCodecs(sdp, type, codecs, mid) {
  if (!codecs || codecs.length === 0) {
    return sdp;
  }

  codecs = type === 'audio' ? codecs.concat(audioCodecAllowList) : codecs.concat(videoCodecAllowList);
  var sdpLines = sdp.split('\r\n');
  var headLines = null;
  var tailLines = null;

  if (typeof mid === 'string') {
    var midRange = findMLineRangeWithMID(sdpLines, mid);

    if (midRange) {
      var start = midRange.start,
          end = midRange.end;
      headLines = sdpLines.slice(0, start);
      tailLines = sdpLines.slice(end);
      sdpLines = sdpLines.slice(start, end);
    }
  } // Search for m line.


  var mLineIndex = findLine(sdpLines, 'm=', type);

  if (mLineIndex === null) {
    return sdp;
  }

  var originPayloads = sdpLines[mLineIndex].split(' ');
  originPayloads.splice(0, 3); // If the codec is available, set it as the default in m line.

  var payloads = [];

  var _iterator2 = _createForOfIteratorHelper(codecs),
      _step2;

  try {
    for (_iterator2.s(); !(_step2 = _iterator2.n()).done;) {
      var codec = _step2.value;

      for (var i = 0; i < sdpLines.length; i++) {
        var index = findLineInRange(sdpLines, i, -1, 'a=rtpmap', codec);

        if (index !== null) {
          var payload = getCodecPayloadTypeFromLine(sdpLines[index]);

          if (payload) {
            payloads.push(payload);
            i = index;
          }
        }
      }
    }
  } catch (err) {
    _iterator2.e(err);
  } finally {
    _iterator2.f();
  }

  payloads = appendRtxPayloads(sdpLines, payloads);
  sdpLines[mLineIndex] = setCodecOrder(sdpLines[mLineIndex], payloads); // Remove a lines.

  var _iterator3 = _createForOfIteratorHelper(originPayloads),
      _step3;

  try {
    for (_iterator3.s(); !(_step3 = _iterator3.n()).done;) {
      var _payload = _step3.value;

      if (payloads.indexOf(_payload) === -1) {
        sdpLines = removeCodecFramALine(sdpLines, _payload);
      }
    }
  } catch (err) {
    _iterator3.e(err);
  } finally {
    _iterator3.f();
  }

  if (headLines) {
    sdpLines = headLines.concat(sdpLines).concat(tailLines);
  }

  sdp = sdpLines.join('\r\n');
  return sdp;
} // Add legacy simulcast.


function addLegacySimulcast(sdp, type, numStreams, mid) {
  var _sdpLines, _sdpLines2;

  if (!numStreams || !(numStreams > 1)) {
    return sdp;
  }

  var sdpLines = sdp.split('\r\n');
  var headLines = null;
  var tailLines = null;

  if (typeof mid === 'string') {
    var midRange = findMLineRangeWithMID(sdpLines, mid);

    if (midRange) {
      var start = midRange.start,
          end = midRange.end;
      headLines = sdpLines.slice(0, start);
      tailLines = sdpLines.slice(end);
      sdpLines = sdpLines.slice(start, end);
    }
  } // Search for m line.


  var mLineStart = findLine(sdpLines, 'm=', type);

  if (mLineStart === null) {
    return sdp;
  }

  var mLineEnd = findLineInRange(sdpLines, mLineStart + 1, -1, 'm=');

  if (mLineEnd === null) {
    mLineEnd = sdpLines.length;
  }

  var ssrcGetter = function ssrcGetter(line) {
    var parts = line.split(' ');
    var ssrc = parts[0].split(':')[1];
    return ssrc;
  }; // Process ssrc lines from mLineIndex.


  var removes = new Set();
  var ssrcs = new Set();
  var gssrcs = new Set();
  var simLines = [];
  var simGroupLines = [];
  var i = mLineStart + 1;

  while (i < mLineEnd) {
    var line = sdpLines[i];

    if (line === '') {
      break;
    }

    if (line.indexOf('a=ssrc:') > -1) {
      var ssrc = ssrcGetter(sdpLines[i]);
      ssrcs.add(ssrc);

      if (line.indexOf('cname') > -1 || line.indexOf('msid') > -1) {
        for (var j = 1; j < numStreams; j++) {
          var nssrc = parseInt(ssrc) + j + '';
          simLines.push(line.replace(ssrc, nssrc));
        }
      } else {
        removes.add(line);
      }
    }

    if (line.indexOf('a=ssrc-group:FID') > -1) {
      var parts = line.split(' ');
      gssrcs.add(parts[2]);

      for (var _j = 1; _j < numStreams; _j++) {
        var nssrc1 = parseInt(parts[1]) + _j + '';
        var nssrc2 = parseInt(parts[2]) + _j + '';
        simGroupLines.push(line.replace(parts[1], nssrc1).replace(parts[2], nssrc2));
      }
    }

    i++;
  }

  var insertPos = i;
  ssrcs.forEach(function (ssrc) {
    if (!gssrcs.has(ssrc)) {
      var groupLine = 'a=ssrc-group:SIM';
      groupLine = groupLine + ' ' + ssrc;

      for (var _j2 = 1; _j2 < numStreams; _j2++) {
        groupLine = groupLine + ' ' + (parseInt(ssrc) + _j2);
      }

      simGroupLines.push(groupLine);
    }
  });
  simLines.sort(); // Insert simulcast ssrc lines.

  (_sdpLines = sdpLines).splice.apply(_sdpLines, [insertPos, 0].concat(simGroupLines));

  (_sdpLines2 = sdpLines).splice.apply(_sdpLines2, [insertPos, 0].concat(simLines));

  sdpLines = sdpLines.filter(function (line) {
    return !removes.has(line);
  });

  if (headLines) {
    sdpLines = headLines.concat(sdpLines).concat(tailLines);
  }

  sdp = sdpLines.join('\r\n');
  return sdp;
}

function setMaxBitrate(sdp, encodingParametersList, mid) {
  var _iterator4 = _createForOfIteratorHelper(encodingParametersList),
      _step4;

  try {
    for (_iterator4.s(); !(_step4 = _iterator4.n()).done;) {
      var encodingParameters = _step4.value;

      if (encodingParameters.maxBitrate) {
        sdp = setCodecParam(sdp, encodingParameters.codec.name, 'x-google-max-bitrate', encodingParameters.maxBitrate.toString(), mid);
      }
    }
  } catch (err) {
    _iterator4.e(err);
  } finally {
    _iterator4.f();
  }

  return sdp;
}

},{"./logger.js":28,"@babel/runtime/helpers/interopRequireDefault":10}],33:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/* global SendStream, BidirectionalStream */
'use strict';

var _interopRequireWildcard = require("@babel/runtime/helpers/interopRequireWildcard");

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.StreamEvent = exports.RemoteStream = exports.LocalStream = exports.Stream = exports.StreamSourceInfo = void 0;

var _typeof2 = _interopRequireDefault(require("@babel/runtime/helpers/typeof"));

var _assertThisInitialized2 = _interopRequireDefault(require("@babel/runtime/helpers/assertThisInitialized"));

var _inherits2 = _interopRequireDefault(require("@babel/runtime/helpers/inherits"));

var _possibleConstructorReturn2 = _interopRequireDefault(require("@babel/runtime/helpers/possibleConstructorReturn"));

var _getPrototypeOf2 = _interopRequireDefault(require("@babel/runtime/helpers/getPrototypeOf"));

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var Utils = _interopRequireWildcard(require("./utils.js"));

var _event = require("./event.js");

function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = (0, _getPrototypeOf2["default"])(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return (0, _possibleConstructorReturn2["default"])(this, result); }; }

function _isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

// eslint-disable-next-line require-jsdoc
function isAllowedValue(obj, allowedValues) {
  return allowedValues.some(function (ele) {
    return ele === obj;
  });
}
/**
 * @class StreamSourceInfo
 * @memberOf Owt.Base
 * @classDesc Information of a stream's source.
 * @constructor
 * @description Audio source info or video source info could be undefined if
 * a stream does not have audio/video track.
 * @param {?string} audioSourceInfo Audio source info. Accepted values are:
 * "mic", "screen-cast", "file", "mixed" or undefined.
 * @param {?string} videoSourceInfo Video source info. Accepted values are:
 * "camera", "screen-cast", "file", "mixed" or undefined.
 * @param {boolean} dataSourceInfo Indicates whether it is data. Accepted values
 * are boolean.
 */


var StreamSourceInfo = // eslint-disable-next-line require-jsdoc
function StreamSourceInfo(audioSourceInfo, videoSourceInfo, dataSourceInfo) {
  (0, _classCallCheck2["default"])(this, StreamSourceInfo);

  if (!isAllowedValue(audioSourceInfo, [undefined, 'mic', 'screen-cast', 'file', 'mixed'])) {
    throw new TypeError('Incorrect value for audioSourceInfo');
  }

  if (!isAllowedValue(videoSourceInfo, [undefined, 'camera', 'screen-cast', 'file', 'encoded-file', 'raw-file', 'mixed'])) {
    throw new TypeError('Incorrect value for videoSourceInfo');
  }

  this.audio = audioSourceInfo;
  this.video = videoSourceInfo;
  this.data = dataSourceInfo;
};
/**
 * @class Stream
 * @memberOf Owt.Base
 * @classDesc Base class of streams.
 * @extends Owt.Base.EventDispatcher
 * @hideconstructor
 */


exports.StreamSourceInfo = StreamSourceInfo;

var Stream = /*#__PURE__*/function (_EventDispatcher) {
  (0, _inherits2["default"])(Stream, _EventDispatcher);

  var _super = _createSuper(Stream);

  // eslint-disable-next-line require-jsdoc
  function Stream(stream, sourceInfo, attributes) {
    var _this;

    (0, _classCallCheck2["default"])(this, Stream);
    _this = _super.call(this);

    if (stream && !(stream instanceof MediaStream) && !(typeof SendStream === 'function' && stream instanceof SendStream) && !(typeof BidirectionalStream === 'function' && stream instanceof BidirectionalStream) || (0, _typeof2["default"])(sourceInfo) !== 'object') {
      throw new TypeError('Invalid stream or sourceInfo.');
    }

    if (stream && stream instanceof MediaStream && (stream.getAudioTracks().length > 0 && !sourceInfo.audio || stream.getVideoTracks().length > 0 && !sourceInfo.video)) {
      throw new TypeError('Missing audio source info or video source info.');
    }
    /**
     * @member {?MediaStream} mediaStream
     * @instance
     * @memberof Owt.Base.Stream
     * @see {@link https://www.w3.org/TR/mediacapture-streams/#mediastream|MediaStream API of Media Capture and Streams}.
     * @desc This property is deprecated, please use stream instead.
     */


    if (stream instanceof MediaStream) {
      Object.defineProperty((0, _assertThisInitialized2["default"])(_this), 'mediaStream', {
        configurable: false,
        writable: true,
        value: stream
      });
    }
    /**
     * @member {MediaStream | SendStream | BidirectionalStream | undefined} stream
     * @instance
     * @memberof Owt.Base.Stream
     * @see {@link https://www.w3.org/TR/mediacapture-streams/#mediastream|MediaStream API of Media Capture and Streams}
     * @see {@link https://wicg.github.io/web-transport/ WebTransport}.
     */


    Object.defineProperty((0, _assertThisInitialized2["default"])(_this), 'stream', {
      configurable: false,
      writable: true,
      value: stream
    });
    /**
     * @member {Owt.Base.StreamSourceInfo} source
     * @instance
     * @memberof Owt.Base.Stream
     * @desc Source info of a stream.
     */

    Object.defineProperty((0, _assertThisInitialized2["default"])(_this), 'source', {
      configurable: false,
      writable: false,
      value: sourceInfo
    });
    /**
     * @member {object} attributes
     * @instance
     * @memberof Owt.Base.Stream
     * @desc Custom attributes of a stream.
     */

    Object.defineProperty((0, _assertThisInitialized2["default"])(_this), 'attributes', {
      configurable: true,
      writable: false,
      value: attributes
    });
    return _this;
  }

  return Stream;
}(_event.EventDispatcher);
/**
 * @class LocalStream
 * @classDesc Stream captured from current endpoint.
 * @memberOf Owt.Base
 * @extends Owt.Base.Stream
 * @constructor
 * @param {MediaStream} stream Underlying MediaStream.
 * @param {Owt.Base.StreamSourceInfo} sourceInfo Information about stream's
 * source.
 * @param {object} attributes Custom attributes of the stream.
 */


exports.Stream = Stream;

var LocalStream = /*#__PURE__*/function (_Stream) {
  (0, _inherits2["default"])(LocalStream, _Stream);

  var _super2 = _createSuper(LocalStream);

  // eslint-disable-next-line require-jsdoc
  function LocalStream(stream, sourceInfo, attributes) {
    var _this2;

    (0, _classCallCheck2["default"])(this, LocalStream);

    if (!stream) {
      throw new TypeError('Stream cannot be null.');
    }

    _this2 = _super2.call(this, stream, sourceInfo, attributes);
    /**
     * @member {string} id
     * @instance
     * @memberof Owt.Base.LocalStream
     */

    Object.defineProperty((0, _assertThisInitialized2["default"])(_this2), 'id', {
      configurable: false,
      writable: false,
      value: Utils.createUuid()
    });
    return _this2;
  }

  return LocalStream;
}(Stream);
/**
 * @class RemoteStream
 * @classDesc Stream sent from a remote endpoint. In conference mode,
 * RemoteStream's stream is always undefined. Please get MediaStream or
 * ReadableStream from subscription's stream property.
 * Events:
 *
 * | Event Name      | Argument Type    | Fired when         |
 * | ----------------| ---------------- | ------------------ |
 * | ended           | Event            | Stream is no longer available on server side.   |
 * | updated         | Event            | Stream is updated. |
 *
 * @memberOf Owt.Base
 * @extends Owt.Base.Stream
 * @hideconstructor
 */


exports.LocalStream = LocalStream;

var RemoteStream = /*#__PURE__*/function (_Stream2) {
  (0, _inherits2["default"])(RemoteStream, _Stream2);

  var _super3 = _createSuper(RemoteStream);

  // eslint-disable-next-line require-jsdoc
  function RemoteStream(id, origin, stream, sourceInfo, attributes) {
    var _this3;

    (0, _classCallCheck2["default"])(this, RemoteStream);
    _this3 = _super3.call(this, stream, sourceInfo, attributes);
    /**
     * @member {string} id
     * @instance
     * @memberof Owt.Base.RemoteStream
     */

    Object.defineProperty((0, _assertThisInitialized2["default"])(_this3), 'id', {
      configurable: false,
      writable: false,
      value: id ? id : Utils.createUuid()
    });
    /**
     * @member {string} origin
     * @instance
     * @memberof Owt.Base.RemoteStream
     * @desc ID of the remote endpoint who published this stream.
     */

    Object.defineProperty((0, _assertThisInitialized2["default"])(_this3), 'origin', {
      configurable: false,
      writable: false,
      value: origin
    });
    /**
     * @member {Owt.Base.PublicationSettings} settings
     * @instance
     * @memberof Owt.Base.RemoteStream
     * @desc Original settings for publishing this stream. This property is only
     * valid in conference mode.
     */

    _this3.settings = undefined;
    /**
     * @member {Owt.Conference.SubscriptionCapabilities} extraCapabilities
     * @instance
     * @memberof Owt.Base.RemoteStream
     * @desc Extra capabilities remote endpoint provides for subscription. Extra
     * capabilities don't include original settings. This property is only valid
     * in conference mode.
     */

    _this3.extraCapabilities = undefined;
    return _this3;
  }

  return RemoteStream;
}(Stream);
/**
 * @class StreamEvent
 * @classDesc Event for Stream.
 * @extends Owt.Base.OwtEvent
 * @memberof Owt.Base
 * @hideconstructor
 */


exports.RemoteStream = RemoteStream;

var StreamEvent = /*#__PURE__*/function (_OwtEvent) {
  (0, _inherits2["default"])(StreamEvent, _OwtEvent);

  var _super4 = _createSuper(StreamEvent);

  // eslint-disable-next-line require-jsdoc
  function StreamEvent(type, init) {
    var _this4;

    (0, _classCallCheck2["default"])(this, StreamEvent);
    _this4 = _super4.call(this, type);
    /**
     * @member {Owt.Base.Stream} stream
     * @instance
     * @memberof Owt.Base.StreamEvent
     */

    _this4.stream = init.stream;
    return _this4;
  }

  return StreamEvent;
}(_event.OwtEvent);

exports.StreamEvent = StreamEvent;

},{"./event.js":26,"./utils.js":35,"@babel/runtime/helpers/assertThisInitialized":3,"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/getPrototypeOf":8,"@babel/runtime/helpers/inherits":9,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/interopRequireWildcard":11,"@babel/runtime/helpers/possibleConstructorReturn":16,"@babel/runtime/helpers/typeof":19}],34:[function(require,module,exports){
// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';
/**
 * @class TransportType
 * @memberOf Owt.Base
 * @classDesc Transport type enumeration.
 * @hideconstructor
 */

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.TransportSettings = exports.TransportConstraints = exports.TransportType = void 0;

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var TransportType = {
  QUIC: 'quic',
  WEBRTC: 'webrtc'
};
/**
 * @class TransportConstraints
 * @memberOf Owt.Base
 * @classDesc Represents the transport constraints for publication and
 * subscription.
 * @hideconstructor
 */

exports.TransportType = TransportType;

var TransportConstraints = // eslint-disable-next-line require-jsdoc
function TransportConstraints(type, id) {
  (0, _classCallCheck2["default"])(this, TransportConstraints);

  /**
   * @member {Array.<Owt.Base.TransportType>} type
   * @instance
   * @memberof Owt.Base.TransportConstraints
   * @desc Transport type for publication and subscription.
   */
  this.type = type;
  /**
   * @member {?string} id
   * @instance
   * @memberof Owt.Base.TransportConstraints
   * @desc Transport ID. Undefined transport ID results server to assign a new
   * one. It should always be undefined if transport type is webrtc since the
   * webrtc agent of OWT server doesn't support multiple transceivers on a
   * single PeerConnection.
   */

  this.id = id;
};
/**
 * @class TransportSettings
 * @memberOf Owt.Base
 * @classDesc Represents the transport settings for publication and
 * subscription.
 * @hideconstructor
 */


exports.TransportConstraints = TransportConstraints;

var TransportSettings = // eslint-disable-next-line require-jsdoc
function TransportSettings(type, id) {
  (0, _classCallCheck2["default"])(this, TransportSettings);

  /**
   * @member {Owt.Base.TransportType} type
   * @instance
   * @memberof Owt.Base.TransportSettings
   * @desc Transport type for publication and subscription.
   */
  this.type = type;
  /**
   * @member {string} id
   * @instance
   * @memberof Owt.Base.TransportSettings
   * @desc Transport ID.
   */

  this.id = id;
  /**
   * @member {?Array.<RTCRtpTransceiver>} rtpTransceivers
   * @instance
   * @memberof Owt.Base.TransportSettings
   * @desc A list of RTCRtpTransceiver associated with the publication or
   * subscription. It's only available in conference mode when TransportType
   * is webrtc.
   * @see {@link https://w3c.github.io/webrtc-pc/#rtcrtptransceiver-interface|RTCRtpTransceiver Interface of WebRTC 1.0}.
   */

  this.rtpTransceivers = undefined;
};

exports.TransportSettings = TransportSettings;

},{"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/interopRequireDefault":10}],35:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/* global navigator, window */
'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.isFirefox = isFirefox;
exports.isChrome = isChrome;
exports.isSafari = isSafari;
exports.isEdge = isEdge;
exports.createUuid = createUuid;
exports.sysInfo = sysInfo;
var sdkVersion = '5.0'; // eslint-disable-next-line require-jsdoc

function isFirefox() {
  return window.navigator.userAgent.match('Firefox') !== null;
} // eslint-disable-next-line require-jsdoc


function isChrome() {
  return window.navigator.userAgent.match('Chrome') !== null;
} // eslint-disable-next-line require-jsdoc


function isSafari() {
  return /^((?!chrome|android).)*safari/i.test(window.navigator.userAgent);
} // eslint-disable-next-line require-jsdoc


function isEdge() {
  return window.navigator.userAgent.match(/Edge\/(\d+).(\d+)$/) !== null;
} // eslint-disable-next-line require-jsdoc


function createUuid() {
  return 'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'.replace(/[xy]/g, function (c) {
    var r = Math.random() * 16 | 0;
    var v = c === 'x' ? r : r & 0x3 | 0x8;
    return v.toString(16);
  });
} // Returns system information.
// Format: {sdk:{version:**, type:**}, runtime:{version:**, name:**}, os:{version:**, name:**}};
// eslint-disable-next-line require-jsdoc


function sysInfo() {
  var info = Object.create({});
  info.sdk = {
    version: sdkVersion,
    type: 'JavaScript'
  }; // Runtime info.

  var userAgent = navigator.userAgent;
  var firefoxRegex = /Firefox\/([0-9.]+)/;
  var chromeRegex = /Chrome\/([0-9.]+)/;
  var edgeRegex = /Edge\/([0-9.]+)/;
  var safariVersionRegex = /Version\/([0-9.]+) Safari/;
  var result = chromeRegex.exec(userAgent);

  if (result) {
    info.runtime = {
      name: 'Chrome',
      version: result[1]
    };
  } else if (result = firefoxRegex.exec(userAgent)) {
    info.runtime = {
      name: 'Firefox',
      version: result[1]
    };
  } else if (result = edgeRegex.exec(userAgent)) {
    info.runtime = {
      name: 'Edge',
      version: result[1]
    };
  } else if (isSafari()) {
    result = safariVersionRegex.exec(userAgent);
    info.runtime = {
      name: 'Safari'
    };
    info.runtime.version = result ? result[1] : 'Unknown';
  } else {
    info.runtime = {
      name: 'Unknown',
      version: 'Unknown'
    };
  } // OS info.


  var windowsRegex = /Windows NT ([0-9.]+)/;
  var macRegex = /Intel Mac OS X ([0-9_.]+)/;
  var iPhoneRegex = /iPhone OS ([0-9_.]+)/;
  var linuxRegex = /X11; Linux/;
  var androidRegex = /Android( ([0-9.]+))?/;
  var chromiumOsRegex = /CrOS/;

  if (result = windowsRegex.exec(userAgent)) {
    info.os = {
      name: 'Windows NT',
      version: result[1]
    };
  } else if (result = macRegex.exec(userAgent)) {
    info.os = {
      name: 'Mac OS X',
      version: result[1].replace(/_/g, '.')
    };
  } else if (result = iPhoneRegex.exec(userAgent)) {
    info.os = {
      name: 'iPhone OS',
      version: result[1].replace(/_/g, '.')
    };
  } else if (result = linuxRegex.exec(userAgent)) {
    info.os = {
      name: 'Linux',
      version: 'Unknown'
    };
  } else if (result = androidRegex.exec(userAgent)) {
    info.os = {
      name: 'Android',
      version: result[1] || 'Unknown'
    };
  } else if (result = chromiumOsRegex.exec(userAgent)) {
    info.os = {
      name: 'Chrome OS',
      version: 'Unknown'
    };
  } else {
    info.os = {
      name: 'Unknown',
      version: 'Unknown'
    };
  }

  info.capabilities = {
    continualIceGathering: false,
    streamRemovable: info.runtime.name !== 'Firefox',
    ignoreDataChannelAcks: true
  };
  return info;
}

},{}],36:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/* eslint-disable require-jsdoc */

/* global Map, Promise, setTimeout */
'use strict';

var _interopRequireWildcard = require("@babel/runtime/helpers/interopRequireWildcard");

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.ConferencePeerConnectionChannel = void 0;

var _slicedToArray2 = _interopRequireDefault(require("@babel/runtime/helpers/slicedToArray"));

var _regenerator = _interopRequireDefault(require("@babel/runtime/regenerator"));

var _typeof2 = _interopRequireDefault(require("@babel/runtime/helpers/typeof"));

var _asyncToGenerator2 = _interopRequireDefault(require("@babel/runtime/helpers/asyncToGenerator"));

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var _createClass2 = _interopRequireDefault(require("@babel/runtime/helpers/createClass"));

var _inherits2 = _interopRequireDefault(require("@babel/runtime/helpers/inherits"));

var _possibleConstructorReturn2 = _interopRequireDefault(require("@babel/runtime/helpers/possibleConstructorReturn"));

var _getPrototypeOf2 = _interopRequireDefault(require("@babel/runtime/helpers/getPrototypeOf"));

var _logger = _interopRequireDefault(require("../base/logger.js"));

var _event = require("../base/event.js");

var _mediaformat = require("../base/mediaformat.js");

var _publication = require("../base/publication.js");

var _subscription = require("./subscription.js");

var _error = require("./error.js");

var Utils = _interopRequireWildcard(require("../base/utils.js"));

var SdpUtils = _interopRequireWildcard(require("../base/sdputils.js"));

var _transport = require("../base/transport.js");

function _createForOfIteratorHelper(o, allowArrayLike) { var it; if (typeof Symbol === "undefined" || o[Symbol.iterator] == null) { if (Array.isArray(o) || (it = _unsupportedIterableToArray(o)) || allowArrayLike && o && typeof o.length === "number") { if (it) o = it; var i = 0; var F = function F() {}; return { s: F, n: function n() { if (i >= o.length) return { done: true }; return { done: false, value: o[i++] }; }, e: function e(_e) { throw _e; }, f: F }; } throw new TypeError("Invalid attempt to iterate non-iterable instance.\nIn order to be iterable, non-array objects must have a [Symbol.iterator]() method."); } var normalCompletion = true, didErr = false, err; return { s: function s() { it = o[Symbol.iterator](); }, n: function n() { var step = it.next(); normalCompletion = step.done; return step; }, e: function e(_e2) { didErr = true; err = _e2; }, f: function f() { try { if (!normalCompletion && it["return"] != null) it["return"](); } finally { if (didErr) throw err; } } }; }

function _unsupportedIterableToArray(o, minLen) { if (!o) return; if (typeof o === "string") return _arrayLikeToArray(o, minLen); var n = Object.prototype.toString.call(o).slice(8, -1); if (n === "Object" && o.constructor) n = o.constructor.name; if (n === "Map" || n === "Set") return Array.from(o); if (n === "Arguments" || /^(?:Ui|I)nt(?:8|16|32)(?:Clamped)?Array$/.test(n)) return _arrayLikeToArray(o, minLen); }

function _arrayLikeToArray(arr, len) { if (len == null || len > arr.length) len = arr.length; for (var i = 0, arr2 = new Array(len); i < len; i++) { arr2[i] = arr[i]; } return arr2; }

function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = (0, _getPrototypeOf2["default"])(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return (0, _possibleConstructorReturn2["default"])(this, result); }; }

function _isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

/**
 * @class ConferencePeerConnectionChannel
 * @classDesc A channel for a connection between client and conference server.
 * Currently, only one stream could be tranmitted in a channel.
 * @hideconstructor
 * @private
 */
var ConferencePeerConnectionChannel = /*#__PURE__*/function (_EventDispatcher) {
  (0, _inherits2["default"])(ConferencePeerConnectionChannel, _EventDispatcher);

  var _super = _createSuper(ConferencePeerConnectionChannel);

  // eslint-disable-next-line require-jsdoc
  function ConferencePeerConnectionChannel(config, signaling) {
    var _this;

    (0, _classCallCheck2["default"])(this, ConferencePeerConnectionChannel);
    _this = _super.call(this);
    _this.pc = null;
    _this._config = config;
    _this._videoCodecs = undefined;
    _this._options = null;
    _this._videoCodecs = undefined;
    _this._signaling = signaling;
    _this._internalId = null; // It's publication ID or subscription ID.

    _this._pendingCandidates = [];
    _this._subscribePromises = new Map(); // internalId => { resolve, reject }

    _this._publishPromises = new Map(); // internalId => { resolve, reject }

    _this._publications = new Map(); // PublicationId => Publication

    _this._subscriptions = new Map(); // SubscriptionId => Subscription

    _this._publishTransceivers = new Map(); // internalId => { id, transceivers: [Transceiver] }

    _this._subscribeTransceivers = new Map(); // internalId => { id, transceivers: [Transceiver] }

    _this._reverseIdMap = new Map(); // PublicationId || SubscriptionId => internalId
    // Timer for PeerConnection disconnected. Will stop connection after timer.

    _this._disconnectTimer = null;
    _this._ended = false; // Channel ID assigned by conference

    _this._id = undefined; // Used to create internal ID for publication/subscription

    _this._internalCount = 0;
    _this._sdpPromise = Promise.resolve();
    _this._sdpResolverMap = new Map(); // internalId => {finish, resolve, reject}

    _this._sdpResolvers = []; // [{finish, resolve, reject}]

    _this._sdpResolveNum = 0;
    _this._remoteMediaStreams = new Map(); // Key is subscription ID, value is MediaStream.

    return _this;
  }
  /**
   * @function onMessage
   * @desc Received a message from conference portal. Defined in client-server
   * protocol.
   * @param {string} notification Notification type.
   * @param {object} message Message received.
   * @private
   */


  (0, _createClass2["default"])(ConferencePeerConnectionChannel, [{
    key: "onMessage",
    value: function onMessage(notification, message) {
      switch (notification) {
        case 'progress':
          if (message.status === 'soac') {
            this._sdpHandler(message.data);
          } else if (message.status === 'ready') {
            this._readyHandler(message.sessionId);
          } else if (message.status === 'error') {
            this._errorHandler(message.sessionId, message.data);
          }

          break;

        case 'stream':
          this._onStreamEvent(message);

          break;

        default:
          _logger["default"].warning('Unknown notification from MCU.');

      }
    }
  }, {
    key: "publishWithTransceivers",
    value: function () {
      var _publishWithTransceivers = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee(stream, transceivers) {
        var _this2 = this;

        var _iterator, _step, _loop, _ret;

        return _regenerator["default"].wrap(function _callee$(_context2) {
          while (1) {
            switch (_context2.prev = _context2.next) {
              case 0:
                _iterator = _createForOfIteratorHelper(transceivers);
                _context2.prev = 1;
                _loop = /*#__PURE__*/_regenerator["default"].mark(function _loop() {
                  var t, transceiverDescription, internalId, offer, trackOptions, publicationId;
                  return _regenerator["default"].wrap(function _loop$(_context) {
                    while (1) {
                      switch (_context.prev = _context.next) {
                        case 0:
                          t = _step.value;

                          if (!(t.direction !== 'sendonly')) {
                            _context.next = 3;
                            break;
                          }

                          return _context.abrupt("return", {
                            v: Promise.reject('RTCRtpTransceiver\'s direction must be sendonly.')
                          });

                        case 3:
                          if (stream.stream.getTracks().includes(t.sender.track)) {
                            _context.next = 5;
                            break;
                          }

                          return _context.abrupt("return", {
                            v: Promise.reject('The track associated with RTCRtpSender is not included in ' + 'stream.')
                          });

                        case 5:
                          if (!(transceivers.length > 2)) {
                            _context.next = 7;
                            break;
                          }

                          return _context.abrupt("return", {
                            v: Promise.reject('At most one transceiver for audio and one transceiver for video ' + 'are accepted.')
                          });

                        case 7:
                          transceiverDescription = transceivers.map(function (t) {
                            var kind = t.sender.track.kind;
                            return {
                              type: kind,
                              transceiver: t,
                              source: stream.source[kind],
                              option: {}
                            };
                          });
                          internalId = _this2._createInternalId();
                          _context.next = 11;
                          return _this2._chainSdpPromise(internalId);

                        case 11:
                          // Copied from publish method.
                          _this2._publishTransceivers.set(internalId, transceiverDescription);

                          _context.next = 14;
                          return _this2.pc.createOffer();

                        case 14:
                          offer = _context.sent;
                          _context.next = 17;
                          return _this2.pc.setLocalDescription(offer);

                        case 17:
                          trackOptions = transceivers.map(function (t) {
                            var kind = t.sender.track.kind;
                            return {
                              type: kind,
                              source: stream.source[kind],
                              mid: t.mid
                            };
                          });
                          _context.next = 20;
                          return _this2._signaling.sendSignalingMessage('publish', {
                            media: {
                              tracks: trackOptions
                            },
                            attributes: stream.attributes,
                            transport: {
                              id: _this2._id,
                              type: 'webrtc'
                            }
                          }).id;

                        case 20:
                          publicationId = _context.sent;
                          _this2._publishTransceivers.get(internalId).id = publicationId;

                          _this2._reverseIdMap.set(publicationId, internalId);

                          _context.next = 25;
                          return _this2._signaling.sendSignalingMessage('soac', {
                            id: _this2._id,
                            signaling: offer
                          });

                        case 25:
                          return _context.abrupt("return", {
                            v: new Promise(function (resolve, reject) {
                              _this2._publishPromises.set(internalId, {
                                resolve: resolve,
                                reject: reject
                              });
                            })
                          });

                        case 26:
                        case "end":
                          return _context.stop();
                      }
                    }
                  }, _loop);
                });

                _iterator.s();

              case 4:
                if ((_step = _iterator.n()).done) {
                  _context2.next = 11;
                  break;
                }

                return _context2.delegateYield(_loop(), "t0", 6);

              case 6:
                _ret = _context2.t0;

                if (!((0, _typeof2["default"])(_ret) === "object")) {
                  _context2.next = 9;
                  break;
                }

                return _context2.abrupt("return", _ret.v);

              case 9:
                _context2.next = 4;
                break;

              case 11:
                _context2.next = 16;
                break;

              case 13:
                _context2.prev = 13;
                _context2.t1 = _context2["catch"](1);

                _iterator.e(_context2.t1);

              case 16:
                _context2.prev = 16;

                _iterator.f();

                return _context2.finish(16);

              case 19:
              case "end":
                return _context2.stop();
            }
          }
        }, _callee, null, [[1, 13, 16, 19]]);
      }));

      function publishWithTransceivers(_x, _x2) {
        return _publishWithTransceivers.apply(this, arguments);
      }

      return publishWithTransceivers;
    }()
  }, {
    key: "publish",
    value: function () {
      var _publish = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee2(stream, options, videoCodecs) {
        var _this3 = this;

        var _iterator2, _step2, parameters, _iterator3, _step3, _parameters, mediaOptions, trackSettings, internalId, offerOptions, transceivers, transceiverInit, transceiver, _parameters2, _transceiverInit, _transceiver, _parameters3, _iterator4, _step4, track, _iterator5, _step5, _track, localDesc;

        return _regenerator["default"].wrap(function _callee2$(_context3) {
          while (1) {
            switch (_context3.prev = _context3.next) {
              case 0:
                if (!this._ended) {
                  _context3.next = 2;
                  break;
                }

                return _context3.abrupt("return", Promise.reject('Connection closed'));

              case 2:
                if (!Array.isArray(options)) {
                  _context3.next = 4;
                  break;
                }

                return _context3.abrupt("return", this.publishWithTransceivers(stream, options));

              case 4:
                if (options === undefined) {
                  options = {
                    audio: !!stream.mediaStream.getAudioTracks().length,
                    video: !!stream.mediaStream.getVideoTracks().length
                  };
                }

                if (!((0, _typeof2["default"])(options) !== 'object')) {
                  _context3.next = 7;
                  break;
                }

                return _context3.abrupt("return", Promise.reject(new TypeError('Options should be an object.')));

              case 7:
                if (!(this._isRtpEncodingParameters(options.audio) && this._isOwtEncodingParameters(options.video) || this._isOwtEncodingParameters(options.audio) && this._isRtpEncodingParameters(options.video))) {
                  _context3.next = 9;
                  break;
                }

                return _context3.abrupt("return", Promise.reject(new _error.ConferenceError('Mixing RTCRtpEncodingParameters and ' + 'AudioEncodingParameters/VideoEncodingParameters is not allowed.')));

              case 9:
                if (options.audio === undefined) {
                  options.audio = !!stream.mediaStream.getAudioTracks().length;
                }

                if (options.video === undefined) {
                  options.video = !!stream.mediaStream.getVideoTracks().length;
                }

                if (!(!!options.audio && !stream.mediaStream.getAudioTracks().length || !!options.video && !stream.mediaStream.getVideoTracks().length)) {
                  _context3.next = 13;
                  break;
                }

                return _context3.abrupt("return", Promise.reject(new _error.ConferenceError('options.audio/video is inconsistent with tracks presented in the ' + 'MediaStream.')));

              case 13:
                if (!((options.audio === false || options.audio === null) && (options.video === false || options.video === null))) {
                  _context3.next = 15;
                  break;
                }

                return _context3.abrupt("return", Promise.reject(new _error.ConferenceError('Cannot publish a stream without audio and video.')));

              case 15:
                if (!((0, _typeof2["default"])(options.audio) === 'object')) {
                  _context3.next = 35;
                  break;
                }

                if (Array.isArray(options.audio)) {
                  _context3.next = 18;
                  break;
                }

                return _context3.abrupt("return", Promise.reject(new TypeError('options.audio should be a boolean or an array.')));

              case 18:
                _iterator2 = _createForOfIteratorHelper(options.audio);
                _context3.prev = 19;

                _iterator2.s();

              case 21:
                if ((_step2 = _iterator2.n()).done) {
                  _context3.next = 27;
                  break;
                }

                parameters = _step2.value;

                if (!(!parameters.codec || typeof parameters.codec.name !== 'string' || parameters.maxBitrate !== undefined && typeof parameters.maxBitrate !== 'number')) {
                  _context3.next = 25;
                  break;
                }

                return _context3.abrupt("return", Promise.reject(new TypeError('options.audio has incorrect parameters.')));

              case 25:
                _context3.next = 21;
                break;

              case 27:
                _context3.next = 32;
                break;

              case 29:
                _context3.prev = 29;
                _context3.t0 = _context3["catch"](19);

                _iterator2.e(_context3.t0);

              case 32:
                _context3.prev = 32;

                _iterator2.f();

                return _context3.finish(32);

              case 35:
                if (!((0, _typeof2["default"])(options.video) === 'object' && !Array.isArray(options.video))) {
                  _context3.next = 37;
                  break;
                }

                return _context3.abrupt("return", Promise.reject(new TypeError('options.video should be a boolean or an array.')));

              case 37:
                if (!this._isOwtEncodingParameters(options.video)) {
                  _context3.next = 55;
                  break;
                }

                _iterator3 = _createForOfIteratorHelper(options.video);
                _context3.prev = 39;

                _iterator3.s();

              case 41:
                if ((_step3 = _iterator3.n()).done) {
                  _context3.next = 47;
                  break;
                }

                _parameters = _step3.value;

                if (!(!_parameters.codec || typeof _parameters.codec.name !== 'string' || _parameters.maxBitrate !== undefined && typeof _parameters.maxBitrate !== 'number' || _parameters.codec.profile !== undefined && typeof _parameters.codec.profile !== 'string')) {
                  _context3.next = 45;
                  break;
                }

                return _context3.abrupt("return", Promise.reject(new TypeError('options.video has incorrect parameters.')));

              case 45:
                _context3.next = 41;
                break;

              case 47:
                _context3.next = 52;
                break;

              case 49:
                _context3.prev = 49;
                _context3.t1 = _context3["catch"](39);

                _iterator3.e(_context3.t1);

              case 52:
                _context3.prev = 52;

                _iterator3.f();

                return _context3.finish(52);

              case 55:
                mediaOptions = {};

                this._createPeerConnection();

                if (!(stream.mediaStream.getAudioTracks().length > 0 && options.audio !== false && options.audio !== null)) {
                  _context3.next = 65;
                  break;
                }

                if (stream.mediaStream.getAudioTracks().length > 1) {
                  _logger["default"].warning('Publishing a stream with multiple audio tracks is not fully' + ' supported.');
                }

                if (!(typeof options.audio !== 'boolean' && (0, _typeof2["default"])(options.audio) !== 'object')) {
                  _context3.next = 61;
                  break;
                }

                return _context3.abrupt("return", Promise.reject(new _error.ConferenceError('Type of audio options should be boolean or an object.')));

              case 61:
                mediaOptions.audio = {};
                mediaOptions.audio.source = stream.source.audio;
                _context3.next = 66;
                break;

              case 65:
                mediaOptions.audio = false;

              case 66:
                if (stream.mediaStream.getVideoTracks().length > 0 && options.video !== false && options.video !== null) {
                  if (stream.mediaStream.getVideoTracks().length > 1) {
                    _logger["default"].warning('Publishing a stream with multiple video tracks is not fully ' + 'supported.');
                  }

                  mediaOptions.video = {};
                  mediaOptions.video.source = stream.source.video;
                  trackSettings = stream.mediaStream.getVideoTracks()[0].getSettings();
                  mediaOptions.video.parameters = {
                    resolution: {
                      width: trackSettings.width,
                      height: trackSettings.height
                    },
                    framerate: trackSettings.frameRate
                  };
                } else {
                  mediaOptions.video = false;
                }

                internalId = this._createInternalId(); // Waiting for previous SDP negotiation if needed

                _context3.next = 70;
                return this._chainSdpPromise(internalId);

              case 70:
                offerOptions = {};
                transceivers = [];

                if (!(typeof this.pc.addTransceiver === 'function')) {
                  _context3.next = 95;
                  break;
                }

                if (!(mediaOptions.audio && stream.mediaStream.getAudioTracks().length > 0)) {
                  _context3.next = 83;
                  break;
                }

                transceiverInit = {
                  direction: 'sendonly',
                  streams: [stream.mediaStream]
                };

                if (this._isRtpEncodingParameters(options.audio)) {
                  transceiverInit.sendEncodings = options.audio;
                }

                transceiver = this.pc.addTransceiver(stream.mediaStream.getAudioTracks()[0], transceiverInit);
                transceivers.push({
                  type: 'audio',
                  transceiver: transceiver,
                  source: mediaOptions.audio.source,
                  option: {
                    audio: options.audio
                  }
                });

                if (!Utils.isFirefox()) {
                  _context3.next = 83;
                  break;
                }

                // Firefox does not support encodings setting in addTransceiver.
                _parameters2 = transceiver.sender.getParameters();
                _parameters2.encodings = transceiverInit.sendEncodings;
                _context3.next = 83;
                return transceiver.sender.setParameters(_parameters2);

              case 83:
                if (!(mediaOptions.video && stream.mediaStream.getVideoTracks().length > 0)) {
                  _context3.next = 93;
                  break;
                }

                _transceiverInit = {
                  direction: 'sendonly',
                  streams: [stream.mediaStream]
                };

                if (this._isRtpEncodingParameters(options.video)) {
                  _transceiverInit.sendEncodings = options.video;
                  this._videoCodecs = videoCodecs;
                }

                _transceiver = this.pc.addTransceiver(stream.mediaStream.getVideoTracks()[0], _transceiverInit);
                transceivers.push({
                  type: 'video',
                  transceiver: _transceiver,
                  source: mediaOptions.video.source,
                  option: {
                    video: options.video
                  }
                });

                if (!Utils.isFirefox()) {
                  _context3.next = 93;
                  break;
                }

                // Firefox does not support encodings setting in addTransceiver.
                _parameters3 = _transceiver.sender.getParameters();
                _parameters3.encodings = _transceiverInit.sendEncodings;
                _context3.next = 93;
                return _transceiver.sender.setParameters(_parameters3);

              case 93:
                _context3.next = 99;
                break;

              case 95:
                // Should not reach here
                if (mediaOptions.audio && stream.mediaStream.getAudioTracks().length > 0) {
                  _iterator4 = _createForOfIteratorHelper(stream.mediaStream.getAudioTracks());

                  try {
                    for (_iterator4.s(); !(_step4 = _iterator4.n()).done;) {
                      track = _step4.value;
                      this.pc.addTrack(track, stream.mediaStream);
                    }
                  } catch (err) {
                    _iterator4.e(err);
                  } finally {
                    _iterator4.f();
                  }
                }

                if (mediaOptions.video && stream.mediaStream.getVideoTracks().length > 0) {
                  _iterator5 = _createForOfIteratorHelper(stream.mediaStream.getVideoTracks());

                  try {
                    for (_iterator5.s(); !(_step5 = _iterator5.n()).done;) {
                      _track = _step5.value;
                      this.pc.addTrack(_track, stream.mediaStream);
                    }
                  } catch (err) {
                    _iterator5.e(err);
                  } finally {
                    _iterator5.f();
                  }
                }

                offerOptions.offerToReceiveAudio = false;
                offerOptions.offerToReceiveVideo = false;

              case 99:
                this._publishTransceivers.set(internalId, {
                  transceivers: transceivers
                });

                this.pc.createOffer(offerOptions).then(function (desc) {
                  localDesc = desc;
                  return _this3.pc.setLocalDescription(desc);
                }).then(function () {
                  var trackOptions = [];
                  transceivers.forEach(function (_ref) {
                    var type = _ref.type,
                        transceiver = _ref.transceiver,
                        source = _ref.source;
                    trackOptions.push({
                      type: type,
                      mid: transceiver.mid,
                      source: source
                    });
                  });
                  return _this3._signaling.sendSignalingMessage('publish', {
                    media: {
                      tracks: trackOptions
                    },
                    attributes: stream.attributes,
                    transport: {
                      id: _this3._id,
                      type: 'webrtc'
                    }
                  })["catch"](function (e) {
                    // Send SDP even when failed to get Answer.
                    _this3._signaling.sendSignalingMessage('soac', {
                      id: _this3._id,
                      signaling: localDesc
                    });

                    throw e;
                  });
                }).then(function (data) {
                  var publicationId = data.id;
                  var messageEvent = new _event.MessageEvent('id', {
                    message: publicationId,
                    origin: _this3._remoteId
                  });

                  _this3.dispatchEvent(messageEvent);

                  _this3._publishTransceivers.get(internalId).id = publicationId;

                  _this3._reverseIdMap.set(publicationId, internalId);

                  if (_this3._id && _this3._id !== data.transportId) {
                    _logger["default"].warning('Server returns conflict ID: ' + data.transportId);
                  }

                  _this3._id = data.transportId; // Modify local SDP before sending

                  if (options) {
                    transceivers.forEach(function (_ref2) {
                      var type = _ref2.type,
                          transceiver = _ref2.transceiver,
                          option = _ref2.option;
                      localDesc.sdp = _this3._setRtpReceiverOptions(localDesc.sdp, option, transceiver.mid);
                      localDesc.sdp = _this3._setRtpSenderOptions(localDesc.sdp, option, transceiver.mid);
                    });
                  }

                  _this3._signaling.sendSignalingMessage('soac', {
                    id: _this3._id,
                    signaling: localDesc
                  });
                })["catch"](function (e) {
                  _logger["default"].error('Failed to create offer or set SDP. Message: ' + e.message);

                  if (_this3._publishTransceivers.get(internalId).id) {
                    _this3._unpublish(internalId);

                    _this3._rejectPromise(e);

                    _this3._fireEndedEventOnPublicationOrSubscription();
                  } else {
                    _this3._unpublish(internalId);
                  }
                });
                return _context3.abrupt("return", new Promise(function (resolve, reject) {
                  _this3._publishPromises.set(internalId, {
                    resolve: resolve,
                    reject: reject
                  });
                }));

              case 102:
              case "end":
                return _context3.stop();
            }
          }
        }, _callee2, this, [[19, 29, 32, 35], [39, 49, 52, 55]]);
      }));

      function publish(_x3, _x4, _x5) {
        return _publish.apply(this, arguments);
      }

      return publish;
    }()
  }, {
    key: "subscribe",
    value: function () {
      var _subscribe = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee3(stream, options) {
        var _this4 = this;

        var mediaOptions, matchedSetting, internalId, offerOptions, transceivers, transceiver, _transceiver2, localDesc;

        return _regenerator["default"].wrap(function _callee3$(_context4) {
          while (1) {
            switch (_context4.prev = _context4.next) {
              case 0:
                if (!this._ended) {
                  _context4.next = 2;
                  break;
                }

                return _context4.abrupt("return", Promise.reject('Connection closed'));

              case 2:
                if (options === undefined) {
                  options = {
                    audio: !!stream.settings.audio,
                    video: !!stream.settings.video
                  };
                }

                if (!((0, _typeof2["default"])(options) !== 'object')) {
                  _context4.next = 5;
                  break;
                }

                return _context4.abrupt("return", Promise.reject(new TypeError('Options should be an object.')));

              case 5:
                if (options.audio === undefined) {
                  options.audio = !!stream.settings.audio;
                }

                if (options.video === undefined) {
                  options.video = !!stream.settings.video;
                }

                if (!(options.audio !== undefined && (0, _typeof2["default"])(options.audio) !== 'object' && typeof options.audio !== 'boolean' && options.audio !== null || options.video !== undefined && (0, _typeof2["default"])(options.video) !== 'object' && typeof options.video !== 'boolean' && options.video !== null)) {
                  _context4.next = 9;
                  break;
                }

                return _context4.abrupt("return", Promise.reject(new TypeError('Invalid options type.')));

              case 9:
                if (!(options.audio && !stream.settings.audio || options.video && !stream.settings.video)) {
                  _context4.next = 11;
                  break;
                }

                return _context4.abrupt("return", Promise.reject(new _error.ConferenceError('options.audio/video cannot be true or an object if there is no ' + 'audio/video track in remote stream.')));

              case 11:
                if (!(options.audio === false && options.video === false)) {
                  _context4.next = 13;
                  break;
                }

                return _context4.abrupt("return", Promise.reject(new _error.ConferenceError('Cannot subscribe a stream without audio and video.')));

              case 13:
                mediaOptions = {};

                if (!options.audio) {
                  _context4.next = 22;
                  break;
                }

                if (!((0, _typeof2["default"])(options.audio) === 'object' && Array.isArray(options.audio.codecs))) {
                  _context4.next = 18;
                  break;
                }

                if (!(options.audio.codecs.length === 0)) {
                  _context4.next = 18;
                  break;
                }

                return _context4.abrupt("return", Promise.reject(new TypeError('Audio codec cannot be an empty array.')));

              case 18:
                mediaOptions.audio = {};
                mediaOptions.audio.from = stream.id;
                _context4.next = 23;
                break;

              case 22:
                mediaOptions.audio = false;

              case 23:
                if (!options.video) {
                  _context4.next = 33;
                  break;
                }

                if (!((0, _typeof2["default"])(options.video) === 'object' && Array.isArray(options.video.codecs))) {
                  _context4.next = 27;
                  break;
                }

                if (!(options.video.codecs.length === 0)) {
                  _context4.next = 27;
                  break;
                }

                return _context4.abrupt("return", Promise.reject(new TypeError('Video codec cannot be an empty array.')));

              case 27:
                mediaOptions.video = {};
                mediaOptions.video.from = stream.id;

                if (options.video.resolution || options.video.frameRate || options.video.bitrateMultiplier && options.video.bitrateMultiplier !== 1 || options.video.keyFrameInterval) {
                  mediaOptions.video.parameters = {
                    resolution: options.video.resolution,
                    framerate: options.video.frameRate,
                    bitrate: options.video.bitrateMultiplier ? 'x' + options.video.bitrateMultiplier.toString() : undefined,
                    keyFrameInterval: options.video.keyFrameInterval
                  };
                }

                if (options.video.rid) {
                  // Use rid matched track ID as from if possible
                  matchedSetting = stream.settings.video.find(function (video) {
                    return video.rid === options.video.rid;
                  });

                  if (matchedSetting && matchedSetting._trackId) {
                    mediaOptions.video.from = matchedSetting._trackId; // Ignore other settings when RID set.

                    delete mediaOptions.video.parameters;
                  }

                  options.video = true;
                }

                _context4.next = 34;
                break;

              case 33:
                mediaOptions.video = false;

              case 34:
                internalId = this._createInternalId(); // Waiting for previous SDP negotiation if needed

                _context4.next = 37;
                return this._chainSdpPromise(internalId);

              case 37:
                offerOptions = {};
                transceivers = [];

                this._createPeerConnection();

                if (typeof this.pc.addTransceiver === 'function') {
                  // |direction| seems not working on Safari.
                  if (mediaOptions.audio) {
                    transceiver = this.pc.addTransceiver('audio', {
                      direction: 'recvonly'
                    });
                    transceivers.push({
                      type: 'audio',
                      transceiver: transceiver,
                      from: mediaOptions.audio.from,
                      option: {
                        audio: options.audio
                      }
                    });
                  }

                  if (mediaOptions.video) {
                    _transceiver2 = this.pc.addTransceiver('video', {
                      direction: 'recvonly'
                    });
                    transceivers.push({
                      type: 'video',
                      transceiver: _transceiver2,
                      from: mediaOptions.video.from,
                      parameters: mediaOptions.video.parameters,
                      option: {
                        video: options.video
                      }
                    });
                  }
                } else {
                  offerOptions.offerToReceiveAudio = !!options.audio;
                  offerOptions.offerToReceiveVideo = !!options.video;
                }

                this._subscribeTransceivers.set(internalId, {
                  transceivers: transceivers
                });

                this.pc.createOffer(offerOptions).then(function (desc) {
                  localDesc = desc;
                  return _this4.pc.setLocalDescription(desc)["catch"](function (errorMessage) {
                    _logger["default"].error('Set local description failed. Message: ' + JSON.stringify(errorMessage));

                    throw errorMessage;
                  });
                }, function (error) {
                  _logger["default"].error('Create offer failed. Error info: ' + JSON.stringify(error));

                  throw error;
                }).then(function () {
                  var trackOptions = [];
                  transceivers.forEach(function (_ref3) {
                    var type = _ref3.type,
                        transceiver = _ref3.transceiver,
                        from = _ref3.from,
                        parameters = _ref3.parameters,
                        option = _ref3.option;
                    trackOptions.push({
                      type: type,
                      mid: transceiver.mid,
                      from: from,
                      parameters: parameters
                    });
                  });
                  return _this4._signaling.sendSignalingMessage('subscribe', {
                    media: {
                      tracks: trackOptions
                    },
                    transport: {
                      id: _this4._id,
                      type: 'webrtc'
                    }
                  })["catch"](function (e) {
                    // Send SDP even when failed to get Answer.
                    _this4._signaling.sendSignalingMessage('soac', {
                      id: _this4._id,
                      signaling: localDesc
                    });

                    throw e;
                  });
                }).then(function (data) {
                  var subscriptionId = data.id;
                  var messageEvent = new _event.MessageEvent('id', {
                    message: subscriptionId,
                    origin: _this4._remoteId
                  });

                  _this4.dispatchEvent(messageEvent);

                  _this4._subscribeTransceivers.get(internalId).id = subscriptionId;

                  _this4._reverseIdMap.set(subscriptionId, internalId);

                  if (_this4._id && _this4._id !== data.transportId) {
                    _logger["default"].warning('Server returns conflict ID: ' + data.transportId);
                  }

                  _this4._id = data.transportId; // Modify local SDP before sending

                  if (options) {
                    transceivers.forEach(function (_ref4) {
                      var type = _ref4.type,
                          transceiver = _ref4.transceiver,
                          option = _ref4.option;
                      localDesc.sdp = _this4._setRtpReceiverOptions(localDesc.sdp, option, transceiver.mid);
                    });
                  }

                  _this4._signaling.sendSignalingMessage('soac', {
                    id: _this4._id,
                    signaling: localDesc
                  });
                })["catch"](function (e) {
                  _logger["default"].error('Failed to create offer or set SDP. Message: ' + e.message);

                  if (_this4._subscribeTransceivers.get(internalId).id) {
                    _this4._unsubscribe(internalId);

                    _this4._rejectPromise(e);

                    _this4._fireEndedEventOnPublicationOrSubscription();
                  } else {
                    _this4._unsubscribe(internalId);
                  }
                });
                return _context4.abrupt("return", new Promise(function (resolve, reject) {
                  _this4._subscribePromises.set(internalId, {
                    resolve: resolve,
                    reject: reject
                  });
                }));

              case 44:
              case "end":
                return _context4.stop();
            }
          }
        }, _callee3, this);
      }));

      function subscribe(_x6, _x7) {
        return _subscribe.apply(this, arguments);
      }

      return subscribe;
    }()
  }, {
    key: "close",
    value: function close() {
      if (this.pc && this.pc.signalingState !== 'closed') {
        this.pc.close();
      }
    }
  }, {
    key: "_chainSdpPromise",
    value: function _chainSdpPromise(internalId) {
      var _this5 = this;

      var prior = this._sdpPromise;
      var negotiationTimeout = 10000;
      this._sdpPromise = prior.then(function () {
        return new Promise(function (resolve, reject) {
          var resolver = {
            finish: false,
            resolve: resolve,
            reject: reject
          };

          _this5._sdpResolvers.push(resolver);

          _this5._sdpResolverMap.set(internalId, resolver);

          setTimeout(function () {
            return reject('Timeout to get SDP answer');
          }, negotiationTimeout);
        });
      });
      return prior["catch"](function (e) {//
      });
    }
  }, {
    key: "_nextSdpPromise",
    value: function _nextSdpPromise() {
      var ret = false; // Skip the finished sdp promise

      while (this._sdpResolveNum < this._sdpResolvers.length) {
        var resolver = this._sdpResolvers[this._sdpResolveNum];
        this._sdpResolveNum++;

        if (!resolver.finish) {
          resolver.resolve();
          resolver.finish = true;
          ret = true;
        }
      }

      return ret;
    }
  }, {
    key: "_createInternalId",
    value: function _createInternalId() {
      return this._internalCount++;
    }
  }, {
    key: "_unpublish",
    value: function _unpublish(internalId) {
      var _this6 = this;

      if (this._publishTransceivers.has(internalId)) {
        var _this$_publishTransce = this._publishTransceivers.get(internalId),
            id = _this$_publishTransce.id,
            transceivers = _this$_publishTransce.transceivers;

        if (id) {
          this._signaling.sendSignalingMessage('unpublish', {
            id: id
          })["catch"](function (e) {
            _logger["default"].warning('MCU returns negative ack for unpublishing, ' + e);
          });

          this._reverseIdMap["delete"](id);
        } // Clean transceiver


        transceivers.forEach(function (_ref5) {
          var transceiver = _ref5.transceiver;

          if (_this6.pc.signalingState === 'stable') {
            transceiver.sender.replaceTrack(null);

            _this6.pc.removeTrack(transceiver.sender);
          }
        });

        this._publishTransceivers["delete"](internalId); // Fire ended event


        if (this._publications.has(id)) {
          var event = new _event.OwtEvent('ended');

          this._publications.get(id).dispatchEvent(event);

          this._publications["delete"](id);
        } else {
          _logger["default"].warning('Invalid publication to unpublish: ' + id);

          if (this._publishPromises.has(internalId)) {
            this._publishPromises.get(internalId).reject(new _error.ConferenceError('Failed to publish'));
          }
        }

        if (this._sdpResolverMap.has(internalId)) {
          var resolver = this._sdpResolverMap.get(internalId);

          if (!resolver.finish) {
            resolver.resolve();
            resolver.finish = true;
          }

          this._sdpResolverMap["delete"](internalId);
        } // Create offer, set local and remote description

      }
    }
  }, {
    key: "_unsubscribe",
    value: function _unsubscribe(internalId) {
      if (this._subscribeTransceivers.has(internalId)) {
        var _this$_subscribeTrans = this._subscribeTransceivers.get(internalId),
            id = _this$_subscribeTrans.id,
            transceivers = _this$_subscribeTrans.transceivers;

        if (id) {
          this._signaling.sendSignalingMessage('unsubscribe', {
            id: id
          })["catch"](function (e) {
            _logger["default"].warning('MCU returns negative ack for unsubscribing, ' + e);
          });
        } // Clean transceiver


        transceivers.forEach(function (_ref6) {
          var transceiver = _ref6.transceiver;
          transceiver.receiver.track.stop();
        });

        this._subscribeTransceivers["delete"](internalId); // Fire ended event


        if (this._subscriptions.has(id)) {
          var event = new _event.OwtEvent('ended');

          this._subscriptions.get(id).dispatchEvent(event);

          this._subscriptions["delete"](id);
        } else {
          _logger["default"].warning('Invalid subscription to unsubscribe: ' + id);

          if (this._subscribePromises.has(internalId)) {
            this._subscribePromises.get(internalId).reject(new _error.ConferenceError('Failed to subscribe'));
          }
        }

        if (this._sdpResolverMap.has(internalId)) {
          var resolver = this._sdpResolverMap.get(internalId);

          if (!resolver.finish) {
            resolver.resolve();
            resolver.finish = true;
          }

          this._sdpResolverMap["delete"](internalId);
        } // Disable media in remote SDP
        // Set remoteDescription and set localDescription

      }
    }
  }, {
    key: "_muteOrUnmute",
    value: function _muteOrUnmute(sessionId, isMute, isPub, trackKind) {
      var _this7 = this;

      var eventName = isPub ? 'stream-control' : 'subscription-control';
      var operation = isMute ? 'pause' : 'play';
      return this._signaling.sendSignalingMessage(eventName, {
        id: sessionId,
        operation: operation,
        data: trackKind
      }).then(function () {
        if (!isPub) {
          var muteEventName = isMute ? 'mute' : 'unmute';

          _this7._subscriptions.get(sessionId).dispatchEvent(new _event.MuteEvent(muteEventName, {
            kind: trackKind
          }));
        }
      });
    }
  }, {
    key: "_applyOptions",
    value: function _applyOptions(sessionId, options) {
      if ((0, _typeof2["default"])(options) !== 'object' || (0, _typeof2["default"])(options.video) !== 'object') {
        return Promise.reject(new _error.ConferenceError('Options should be an object.'));
      }

      var videoOptions = {};
      videoOptions.resolution = options.video.resolution;
      videoOptions.framerate = options.video.frameRate;
      videoOptions.bitrate = options.video.bitrateMultiplier ? 'x' + options.video.bitrateMultiplier.toString() : undefined;
      videoOptions.keyFrameInterval = options.video.keyFrameInterval;
      return this._signaling.sendSignalingMessage('subscription-control', {
        id: sessionId,
        operation: 'update',
        data: {
          video: {
            parameters: videoOptions
          }
        }
      }).then();
    }
  }, {
    key: "_onRemoteStreamAdded",
    value: function _onRemoteStreamAdded(event) {
      _logger["default"].debug('Remote stream added.');

      var _iterator6 = _createForOfIteratorHelper(this._subscribeTransceivers),
          _step6;

      try {
        for (_iterator6.s(); !(_step6 = _iterator6.n()).done;) {
          var _step6$value = (0, _slicedToArray2["default"])(_step6.value, 2),
              internalId = _step6$value[0],
              sub = _step6$value[1];

          if (sub.transceivers.find(function (t) {
            return t.transceiver === event.transceiver;
          })) {
            if (this._subscriptions.has(sub.id)) {
              var subscription = this._subscriptions.get(sub.id);

              subscription.stream = event.streams[0];

              if (this._subscribePromises.has(internalId)) {
                this._subscribePromises.get(internalId).resolve(subscription);

                this._subscribePromises["delete"](internalId);
              }
            } else {
              this._remoteMediaStreams.set(sub.id, event.streams[0]);
            }

            return;
          }
        } // This is not expected path. However, this is going to happen on Safari
        // because it does not support setting direction of transceiver.

      } catch (err) {
        _iterator6.e(err);
      } finally {
        _iterator6.f();
      }

      _logger["default"].warning('Received remote stream without subscription.');
    }
  }, {
    key: "_onLocalIceCandidate",
    value: function _onLocalIceCandidate(event) {
      if (event.candidate) {
        if (this.pc.signalingState !== 'stable') {
          this._pendingCandidates.push(event.candidate);
        } else {
          this._sendCandidate(event.candidate);
        }
      } else {
        _logger["default"].debug('Empty candidate.');
      }
    }
  }, {
    key: "_fireEndedEventOnPublicationOrSubscription",
    value: function _fireEndedEventOnPublicationOrSubscription() {
      if (this._ended) {
        return;
      }

      this._ended = true;
      var event = new _event.OwtEvent('ended');

      var _iterator7 = _createForOfIteratorHelper(this._publications),
          _step7;

      try {
        for (_iterator7.s(); !(_step7 = _iterator7.n()).done;) {
          var _step7$value = (0, _slicedToArray2["default"])(_step7.value, 2),

          /* id */
          publication = _step7$value[1];

          publication.dispatchEvent(event);
          publication.stop();
        }
      } catch (err) {
        _iterator7.e(err);
      } finally {
        _iterator7.f();
      }

      var _iterator8 = _createForOfIteratorHelper(this._subscriptions),
          _step8;

      try {
        for (_iterator8.s(); !(_step8 = _iterator8.n()).done;) {
          var _step8$value = (0, _slicedToArray2["default"])(_step8.value, 2),

          /* id */
          subscription = _step8$value[1];

          subscription.dispatchEvent(event);
          subscription.stop();
        }
      } catch (err) {
        _iterator8.e(err);
      } finally {
        _iterator8.f();
      }

      this.dispatchEvent(event);
      this.close();
    }
  }, {
    key: "_rejectPromise",
    value: function _rejectPromise(error) {
      if (!error) {
        error = new _error.ConferenceError('Connection failed or closed.');
      }

      if (this.pc && this.pc.iceConnectionState !== 'closed') {
        this.pc.close();
      } // Rejecting all corresponding promises if publishing and subscribing is ongoing.


      var _iterator9 = _createForOfIteratorHelper(this._publishPromises),
          _step9;

      try {
        for (_iterator9.s(); !(_step9 = _iterator9.n()).done;) {
          var _step9$value = (0, _slicedToArray2["default"])(_step9.value, 2),

          /* id */
          promise = _step9$value[1];

          promise.reject(error);
        }
      } catch (err) {
        _iterator9.e(err);
      } finally {
        _iterator9.f();
      }

      this._publishPromises.clear();

      var _iterator10 = _createForOfIteratorHelper(this._subscribePromises),
          _step10;

      try {
        for (_iterator10.s(); !(_step10 = _iterator10.n()).done;) {
          var _step10$value = (0, _slicedToArray2["default"])(_step10.value, 2),

          /* id */
          _promise = _step10$value[1];

          _promise.reject(error);
        }
      } catch (err) {
        _iterator10.e(err);
      } finally {
        _iterator10.f();
      }

      this._subscribePromises.clear();
    }
  }, {
    key: "_onIceConnectionStateChange",
    value: function _onIceConnectionStateChange(event) {
      if (!event || !event.currentTarget) {
        return;
      }

      _logger["default"].debug('ICE connection state changed to ' + event.currentTarget.iceConnectionState);

      if (event.currentTarget.iceConnectionState === 'closed' || event.currentTarget.iceConnectionState === 'failed') {
        if (event.currentTarget.iceConnectionState === 'failed') {
          this._handleError('connection failed.');
        } else {
          // Fire ended event if publication or subscription exists.
          this._fireEndedEventOnPublicationOrSubscription();
        }
      }
    }
  }, {
    key: "_onConnectionStateChange",
    value: function _onConnectionStateChange(event) {
      if (this.pc.connectionState === 'closed' || this.pc.connectionState === 'failed') {
        if (this.pc.connectionState === 'failed') {
          this._handleError('connection failed.');
        } else {
          // Fire ended event if publication or subscription exists.
          this._fireEndedEventOnPublicationOrSubscription();
        }
      }
    }
  }, {
    key: "_sendCandidate",
    value: function _sendCandidate(candidate) {
      this._signaling.sendSignalingMessage('soac', {
        id: this._id,
        signaling: {
          type: 'candidate',
          candidate: {
            candidate: 'a=' + candidate.candidate,
            sdpMid: candidate.sdpMid,
            sdpMLineIndex: candidate.sdpMLineIndex
          }
        }
      });
    }
  }, {
    key: "_createPeerConnection",
    value: function _createPeerConnection() {
      var _this8 = this;

      if (this.pc) {
        return;
      }

      var pcConfiguration = this._config.rtcConfiguration || {};

      if (Utils.isChrome()) {
        pcConfiguration.bundlePolicy = 'max-bundle';
      }

      this.pc = new RTCPeerConnection(pcConfiguration);

      this.pc.onicecandidate = function (event) {
        _this8._onLocalIceCandidate.apply(_this8, [event]);
      };

      this.pc.ontrack = function (event) {
        _this8._onRemoteStreamAdded.apply(_this8, [event]);
      };

      this.pc.oniceconnectionstatechange = function (event) {
        _this8._onIceConnectionStateChange.apply(_this8, [event]);
      };

      this.pc.onconnectionstatechange = function (event) {
        _this8._onConnectionStateChange.apply(_this8, [event]);
      };
    }
  }, {
    key: "_getStats",
    value: function _getStats() {
      if (this.pc) {
        return this.pc.getStats();
      } else {
        return Promise.reject(new _error.ConferenceError('PeerConnection is not available.'));
      }
    }
  }, {
    key: "_readyHandler",
    value: function _readyHandler(sessionId) {
      var _this9 = this;

      var internalId = this._reverseIdMap.get(sessionId);

      if (this._subscribePromises.has(internalId)) {
        var mediaStream = this._remoteMediaStreams.get(sessionId);

        var transportSettings = new _transport.TransportSettings(_transport.TransportType.WEBRTC, this._id);
        transportSettings.rtpTransceivers = this._subscribeTransceivers.get(internalId).transceivers;
        var subscription = new _subscription.Subscription(sessionId, mediaStream, transportSettings, function () {
          _this9._unsubscribe(internalId);
        }, function () {
          return _this9._getStats();
        }, function (trackKind) {
          return _this9._muteOrUnmute(sessionId, true, false, trackKind);
        }, function (trackKind) {
          return _this9._muteOrUnmute(sessionId, false, false, trackKind);
        }, function (options) {
          return _this9._applyOptions(sessionId, options);
        });

        this._subscriptions.set(sessionId, subscription); // Resolve subscription if mediaStream is ready.


        if (this._subscriptions.get(sessionId).stream) {
          this._subscribePromises.get(internalId).resolve(subscription);

          this._subscribePromises["delete"](internalId);
        }
      } else if (this._publishPromises.has(internalId)) {
        var _transportSettings = new _transport.TransportSettings(_transport.TransportType.WEBRTC, this._id);

        _transportSettings.transceivers = this._publishTransceivers.get(internalId).transceivers;
        var publication = new _publication.Publication(sessionId, _transportSettings, function () {
          _this9._unpublish(internalId);

          return Promise.resolve();
        }, function () {
          return _this9._getStats();
        }, function (trackKind) {
          return _this9._muteOrUnmute(sessionId, true, true, trackKind);
        }, function (trackKind) {
          return _this9._muteOrUnmute(sessionId, false, true, trackKind);
        });

        this._publications.set(sessionId, publication);

        this._publishPromises.get(internalId).resolve(publication); // Do not fire publication's ended event when associated stream is ended.
        // It may still sending silence or black frames.
        // Refer to https://w3c.github.io/webrtc-pc/#rtcrtpsender-interface.

      } else if (!sessionId) {// Channel ready
      }
    }
  }, {
    key: "_sdpHandler",
    value: function _sdpHandler(sdp) {
      var _this10 = this;

      if (sdp.type === 'answer') {
        this.pc.setRemoteDescription(sdp).then(function () {
          if (_this10._pendingCandidates.length > 0) {
            var _iterator11 = _createForOfIteratorHelper(_this10._pendingCandidates),
                _step11;

            try {
              for (_iterator11.s(); !(_step11 = _iterator11.n()).done;) {
                var candidate = _step11.value;

                _this10._sendCandidate(candidate);
              }
            } catch (err) {
              _iterator11.e(err);
            } finally {
              _iterator11.f();
            }
          }
        }, function (error) {
          _logger["default"].error('Set remote description failed: ' + error);

          _this10._rejectPromise(error);

          _this10._fireEndedEventOnPublicationOrSubscription();
        }).then(function () {
          if (!_this10._nextSdpPromise()) {
            _logger["default"].warning('Unexpected SDP promise state');
          }
        });
      }
    }
  }, {
    key: "_errorHandler",
    value: function _errorHandler(sessionId, errorMessage) {
      if (!sessionId) {
        // Transport error
        return this._handleError(errorMessage);
      } // Fire error event on publication or subscription


      var errorEvent = new _event.ErrorEvent('error', {
        error: new _error.ConferenceError(errorMessage)
      });

      if (this._publications.has(sessionId)) {
        this._publications.get(sessionId).dispatchEvent(errorEvent);
      }

      if (this._subscriptions.has(sessionId)) {
        this._subscriptions.get(sessionId).dispatchEvent(errorEvent);
      } // Stop publication or subscription


      var internalId = this._reverseIdMap.get(sessionId);

      if (this._publishTransceivers.has(internalId)) {
        this._unpublish(internalId);
      }

      if (this._subscribeTransceivers.has(internalId)) {
        this._unsubscribe(internalId);
      }
    }
  }, {
    key: "_handleError",
    value: function _handleError(errorMessage) {
      var error = new _error.ConferenceError(errorMessage);

      if (this._ended) {
        return;
      }

      var errorEvent = new _event.ErrorEvent('error', {
        error: error
      });

      var _iterator12 = _createForOfIteratorHelper(this._publications),
          _step12;

      try {
        for (_iterator12.s(); !(_step12 = _iterator12.n()).done;) {
          var _step12$value = (0, _slicedToArray2["default"])(_step12.value, 2),

          /* id */
          publication = _step12$value[1];

          publication.dispatchEvent(errorEvent);
        }
      } catch (err) {
        _iterator12.e(err);
      } finally {
        _iterator12.f();
      }

      var _iterator13 = _createForOfIteratorHelper(this._subscriptions),
          _step13;

      try {
        for (_iterator13.s(); !(_step13 = _iterator13.n()).done;) {
          var _step13$value = (0, _slicedToArray2["default"])(_step13.value, 2),

          /* id */
          subscription = _step13$value[1];

          subscription.dispatchEvent(errorEvent);
        } // Fire ended event when error occured

      } catch (err) {
        _iterator13.e(err);
      } finally {
        _iterator13.f();
      }

      this._fireEndedEventOnPublicationOrSubscription();
    }
  }, {
    key: "_setCodecOrder",
    value: function _setCodecOrder(sdp, options, mid) {
      if (options.audio) {
        if (options.audio.codecs) {
          var audioCodecNames = Array.from(options.audio.codecs, function (codec) {
            return codec.name;
          });
          sdp = SdpUtils.reorderCodecs(sdp, 'audio', audioCodecNames, mid);
        } else {
          var _audioCodecNames = Array.from(options.audio, function (encodingParameters) {
            return encodingParameters.codec.name;
          });

          sdp = SdpUtils.reorderCodecs(sdp, 'audio', _audioCodecNames, mid);
        }
      }

      if (options.video) {
        if (options.video.codecs) {
          var videoCodecNames = Array.from(options.video.codecs, function (codec) {
            return codec.name;
          });
          sdp = SdpUtils.reorderCodecs(sdp, 'video', videoCodecNames, mid);
        } else {
          var _videoCodecNames = Array.from(options.video, function (encodingParameters) {
            return encodingParameters.codec.name;
          });

          sdp = SdpUtils.reorderCodecs(sdp, 'video', _videoCodecNames, mid);
        }
      }

      return sdp;
    }
  }, {
    key: "_setMaxBitrate",
    value: function _setMaxBitrate(sdp, options, mid) {
      if ((0, _typeof2["default"])(options.audio) === 'object') {
        sdp = SdpUtils.setMaxBitrate(sdp, options.audio, mid);
      }

      if ((0, _typeof2["default"])(options.video) === 'object') {
        sdp = SdpUtils.setMaxBitrate(sdp, options.video, mid);
      }

      return sdp;
    }
  }, {
    key: "_setRtpSenderOptions",
    value: function _setRtpSenderOptions(sdp, options, mid) {
      // SDP mugling is deprecated, moving to `setParameters`.
      if (this._isRtpEncodingParameters(options.audio) || this._isRtpEncodingParameters(options.video)) {
        return sdp;
      }

      sdp = this._setMaxBitrate(sdp, options, mid);
      return sdp;
    }
  }, {
    key: "_setRtpReceiverOptions",
    value: function _setRtpReceiverOptions(sdp, options, mid) {
      // Add legacy simulcast in SDP for safari.
      if (this._isRtpEncodingParameters(options.video) && Utils.isSafari()) {
        if (options.video.length > 1) {
          sdp = SdpUtils.addLegacySimulcast(sdp, 'video', options.video.length, mid);
        }
      } // _videoCodecs is a workaround for setting video codecs. It will be moved to RTCRtpSendParameters.


      if (this._isRtpEncodingParameters(options.video) && this._videoCodecs) {
        sdp = SdpUtils.reorderCodecs(sdp, 'video', this._videoCodecs, mid);
        return sdp;
      }

      if (this._isRtpEncodingParameters(options.audio) || this._isRtpEncodingParameters(options.video)) {
        return sdp;
      }

      sdp = this._setCodecOrder(sdp, options, mid);
      return sdp;
    } // Handle stream event sent from MCU. Some stream update events sent from
    // server, more specifically audio.status and video.status events should be
    // publication event or subscription events. They don't change MediaStream's
    // status. See
    // https://github.com/open-webrtc-toolkit/owt-server/blob/master/doc/Client-Portal%20Protocol.md#339-participant-is-notified-on-streams-update-in-room
    // for more information.

  }, {
    key: "_onStreamEvent",
    value: function _onStreamEvent(message) {
      var eventTargets = [];

      if (this._publications.has(message.id)) {
        eventTargets.push(this._publications.get(message.id));
      }

      var _iterator14 = _createForOfIteratorHelper(this._subscriptions),
          _step14;

      try {
        for (_iterator14.s(); !(_step14 = _iterator14.n()).done;) {
          var subscription = _step14.value;

          if (message.id === subscription._audioTrackId || message.id === subscription._videoTrackId) {
            eventTargets.push(subscription);
          }
        }
      } catch (err) {
        _iterator14.e(err);
      } finally {
        _iterator14.f();
      }

      if (!eventTargets.length) {
        return;
      }

      var trackKind;

      if (message.data.field === 'audio.status') {
        trackKind = _mediaformat.TrackKind.AUDIO;
      } else if (message.data.field === 'video.status') {
        trackKind = _mediaformat.TrackKind.VIDEO;
      } else {
        _logger["default"].warning('Invalid data field for stream update info.');
      }

      if (message.data.value === 'active') {
        eventTargets.forEach(function (target) {
          return target.dispatchEvent(new _event.MuteEvent('unmute', {
            kind: trackKind
          }));
        });
      } else if (message.data.value === 'inactive') {
        eventTargets.forEach(function (target) {
          return target.dispatchEvent(new _event.MuteEvent('mute', {
            kind: trackKind
          }));
        });
      } else {
        _logger["default"].warning('Invalid data value for stream update info.');
      }
    }
  }, {
    key: "_isRtpEncodingParameters",
    value: function _isRtpEncodingParameters(obj) {
      if (!Array.isArray(obj)) {
        return false;
      } // Only check the first one.


      var param = obj[0];
      return !!(param.codecPayloadType || param.dtx || param.active || param.ptime || param.maxFramerate || param.scaleResolutionDownBy || param.rid || param.scalabilityMode);
    }
  }, {
    key: "_isOwtEncodingParameters",
    value: function _isOwtEncodingParameters(obj) {
      if (!Array.isArray(obj)) {
        return false;
      } // Only check the first one.


      var param = obj[0];
      return !!param.codec;
    }
  }]);
  return ConferencePeerConnectionChannel;
}(_event.EventDispatcher);

exports.ConferencePeerConnectionChannel = ConferencePeerConnectionChannel;

},{"../base/event.js":26,"../base/logger.js":28,"../base/mediaformat.js":29,"../base/publication.js":31,"../base/sdputils.js":32,"../base/transport.js":34,"../base/utils.js":35,"./error.js":38,"./subscription.js":46,"@babel/runtime/helpers/asyncToGenerator":4,"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/createClass":7,"@babel/runtime/helpers/getPrototypeOf":8,"@babel/runtime/helpers/inherits":9,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/interopRequireWildcard":11,"@babel/runtime/helpers/possibleConstructorReturn":16,"@babel/runtime/helpers/slicedToArray":18,"@babel/runtime/helpers/typeof":19,"@babel/runtime/regenerator":22}],37:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/* global Map, Promise */
'use strict';

var _interopRequireWildcard = require("@babel/runtime/helpers/interopRequireWildcard");

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.ConferenceClient = void 0;

var _regenerator = _interopRequireDefault(require("@babel/runtime/regenerator"));

var _asyncToGenerator2 = _interopRequireDefault(require("@babel/runtime/helpers/asyncToGenerator"));

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var EventModule = _interopRequireWildcard(require("../base/event.js"));

var _signaling = require("./signaling.js");

var _logger = _interopRequireDefault(require("../base/logger.js"));

var _base = require("../base/base64.js");

var _error = require("./error.js");

var Utils = _interopRequireWildcard(require("../base/utils.js"));

var StreamModule = _interopRequireWildcard(require("../base/stream.js"));

var _participant2 = require("./participant.js");

var _info = require("./info.js");

var _channel = require("./channel.js");

var _quicconnection = require("./quicconnection.js");

var _mixedstream = require("./mixedstream.js");

var StreamUtilsModule = _interopRequireWildcard(require("./streamutils.js"));

function _createForOfIteratorHelper(o, allowArrayLike) { var it; if (typeof Symbol === "undefined" || o[Symbol.iterator] == null) { if (Array.isArray(o) || (it = _unsupportedIterableToArray(o)) || allowArrayLike && o && typeof o.length === "number") { if (it) o = it; var i = 0; var F = function F() {}; return { s: F, n: function n() { if (i >= o.length) return { done: true }; return { done: false, value: o[i++] }; }, e: function e(_e) { throw _e; }, f: F }; } throw new TypeError("Invalid attempt to iterate non-iterable instance.\nIn order to be iterable, non-array objects must have a [Symbol.iterator]() method."); } var normalCompletion = true, didErr = false, err; return { s: function s() { it = o[Symbol.iterator](); }, n: function n() { var step = it.next(); normalCompletion = step.done; return step; }, e: function e(_e2) { didErr = true; err = _e2; }, f: function f() { try { if (!normalCompletion && it["return"] != null) it["return"](); } finally { if (didErr) throw err; } } }; }

function _unsupportedIterableToArray(o, minLen) { if (!o) return; if (typeof o === "string") return _arrayLikeToArray(o, minLen); var n = Object.prototype.toString.call(o).slice(8, -1); if (n === "Object" && o.constructor) n = o.constructor.name; if (n === "Map" || n === "Set") return Array.from(o); if (n === "Arguments" || /^(?:Ui|I)nt(?:8|16|32)(?:Clamped)?Array$/.test(n)) return _arrayLikeToArray(o, minLen); }

function _arrayLikeToArray(arr, len) { if (len == null || len > arr.length) len = arr.length; for (var i = 0, arr2 = new Array(len); i < len; i++) { arr2[i] = arr[i]; } return arr2; }

var SignalingState = {
  READY: 1,
  CONNECTING: 2,
  CONNECTED: 3
};
var protocolVersion = '1.2';
/* eslint-disable valid-jsdoc */

/**
 * @class ParticipantEvent
 * @classDesc Class ParticipantEvent represents a participant event.
   @extends Owt.Base.OwtEvent
 * @memberof Owt.Conference
 * @hideconstructor
 */

var ParticipantEvent = function ParticipantEvent(type, init) {
  var that = new EventModule.OwtEvent(type, init);
  /**
   * @member {Owt.Conference.Participant} participant
   * @instance
   * @memberof Owt.Conference.ParticipantEvent
   */

  that.participant = init.participant;
  return that;
};
/* eslint-enable valid-jsdoc */

/**
 * @class ConferenceClientConfiguration
 * @classDesc Configuration for ConferenceClient.
 * @memberOf Owt.Conference
 * @hideconstructor
 */


var ConferenceClientConfiguration = // eslint-disable-line no-unused-vars
// eslint-disable-next-line require-jsdoc
function ConferenceClientConfiguration() {
  (0, _classCallCheck2["default"])(this, ConferenceClientConfiguration);

  /**
   * @member {?RTCConfiguration} rtcConfiguration
   * @instance
   * @memberof Owt.Conference.ConferenceClientConfiguration
   * @desc It will be used for creating PeerConnection.
   * @see {@link https://www.w3.org/TR/webrtc/#rtcconfiguration-dictionary|RTCConfiguration Dictionary of WebRTC 1.0}.
   * @example
   * // Following object can be set to conferenceClientConfiguration.rtcConfiguration.
   * {
   *   iceServers: [{
   *      urls: "stun:example.com:3478"
   *   }, {
   *     urls: [
   *       "turn:example.com:3478?transport=udp",
   *       "turn:example.com:3478?transport=tcp"
   *     ],
   *      credential: "password",
   *      username: "username"
   *   }
   * }
   */
  this.rtcConfiguration = undefined;
  /**
   * @member {?WebTransportOptions} webTransportConfiguration
   * @instance
   * @memberof Owt.Conference.ConferenceClientConfiguration
   * @desc It will be used for creating WebTransport.
   * @see {@link https://w3c.github.io/webtransport/#dictdef-webtransportoptions|WebTransportOptions of WebTransport}.
   * @example
   * // Following object can be set to conferenceClientConfiguration.webTransportConfiguration.
   * {
   *   serverCertificateFingerprints: [{
   *     value:
   *         '00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF',
   *     algorithm: 'sha-256',
   *   }],
   * }
   */

  this.webTransportConfiguration = undefined;
};
/**
 * @class ConferenceClient
 * @classdesc The ConferenceClient handles PeerConnections between client and server. For conference controlling, please refer to REST API guide.
 * Events:
 *
 * | Event Name            | Argument Type                    | Fired when       |
 * | --------------------- | ---------------------------------| ---------------- |
 * | streamadded           | Owt.Base.StreamEvent             | A new stream is available in the conference. |
 * | participantjoined     | Owt.Conference.ParticipantEvent  | A new participant joined the conference. |
 * | messagereceived       | Owt.Base.MessageEvent            | A new message is received. |
 * | serverdisconnected    | Owt.Base.OwtEvent                | Disconnected from conference server. |
 *
 * @memberof Owt.Conference
 * @extends Owt.Base.EventDispatcher
 * @constructor
 * @param {?Owt.Conference.ConferenceClientConfiguration } config Configuration for ConferenceClient.
 * @param {?Owt.Conference.SioSignaling } signalingImpl Signaling channel implementation for ConferenceClient. SDK uses default signaling channel implementation if this parameter is undefined. Currently, a Socket.IO signaling channel implementation was provided as ics.conference.SioSignaling. However, it is not recommended to directly access signaling channel or customize signaling channel for ConferenceClient as this time.
 */


var ConferenceClient = function ConferenceClient(config, signalingImpl) {
  Object.setPrototypeOf(this, new EventModule.EventDispatcher());
  config = config || {};
  var self = this;
  var signalingState = SignalingState.READY;
  var signaling = signalingImpl ? signalingImpl : new _signaling.SioSignaling();
  var me;
  var room;
  var remoteStreams = new Map(); // Key is stream ID, value is a RemoteStream.

  var participants = new Map(); // Key is participant ID, value is a Participant object.

  var publishChannels = new Map(); // Key is MediaStream's ID, value is pc channel.

  var channels = new Map(); // Key is channel's internal ID, value is channel.

  var peerConnectionChannel = null; // PeerConnection for WebRTC.

  var quicTransportChannel = null;
  /**
   * @member {RTCPeerConnection} peerConnection
   * @instance
   * @readonly
   * @desc PeerConnection for WebRTC connection with conference server.
   * @memberof Owt.Conference.ConferenceClient
   * @see {@link https://w3c.github.io/webrtc-pc/#rtcpeerconnection-interface|RTCPeerConnection Interface of WebRTC 1.0}.
   */

  Object.defineProperty(this, 'peerConnection', {
    configurable: false,
    get: function get() {
      var _peerConnectionChanne;

      return (_peerConnectionChanne = peerConnectionChannel) === null || _peerConnectionChanne === void 0 ? void 0 : _peerConnectionChanne.pc;
    }
  });
  /**
   * @function onSignalingMessage
   * @desc Received a message from conference portal. Defined in client-server protocol.
   * @param {string} notification Notification type.
   * @param {object} data Data received.
   * @private
   */

  function onSignalingMessage(notification, data) {
    if (notification === 'soac' || notification === 'progress') {
      if (channels.has(data.id)) {
        channels.get(data.id).onMessage(notification, data);
      } else if (quicTransportChannel && quicTransportChannel.hasContentSessionId(data.id)) {
        quicTransportChannel.onMessage(notification, data);
      } else {
        _logger["default"].warning('Cannot find a channel for incoming data.');
      }

      return;
    } else if (notification === 'stream') {
      if (data.status === 'add') {
        fireStreamAdded(data.data);
      } else if (data.status === 'remove') {
        fireStreamRemoved(data);
      } else if (data.status === 'update') {
        // Broadcast audio/video update status to channel so specific events can be fired on publication or subscription.
        if (data.data.field === 'audio.status' || data.data.field === 'video.status') {
          channels.forEach(function (c) {
            c.onMessage(notification, data);
          });
        } else if (data.data.field === 'activeInput') {
          fireActiveAudioInputChange(data);
        } else if (data.data.field === 'video.layout') {
          fireLayoutChange(data);
        } else if (data.data.field === '.') {
          updateRemoteStream(data.data.value);
        } else {
          _logger["default"].warning('Unknown stream event from MCU.');
        }
      }
    } else if (notification === 'text') {
      fireMessageReceived(data);
    } else if (notification === 'participant') {
      fireParticipantEvent(data);
    }
  }

  signaling.addEventListener('data', function (event) {
    onSignalingMessage(event.message.notification, event.message.data);
  });
  signaling.addEventListener('disconnect', function () {
    clean();
    signalingState = SignalingState.READY;
    self.dispatchEvent(new EventModule.OwtEvent('serverdisconnected'));
  }); // eslint-disable-next-line require-jsdoc

  function fireParticipantEvent(data) {
    if (data.action === 'join') {
      data = data.data;
      var participant = new _participant2.Participant(data.id, data.role, data.user);
      participants.set(data.id, participant);
      var event = new ParticipantEvent('participantjoined', {
        participant: participant
      });
      self.dispatchEvent(event);
    } else if (data.action === 'leave') {
      var participantId = data.data;

      if (!participants.has(participantId)) {
        _logger["default"].warning('Received leave message from MCU for an unknown participant.');

        return;
      }

      var _participant = participants.get(participantId);

      participants["delete"](participantId);

      _participant.dispatchEvent(new EventModule.OwtEvent('left'));
    }
  } // eslint-disable-next-line require-jsdoc


  function fireMessageReceived(data) {
    var messageEvent = new EventModule.MessageEvent('messagereceived', {
      message: data.message,
      origin: data.from,
      to: data.to
    });
    self.dispatchEvent(messageEvent);
  } // eslint-disable-next-line require-jsdoc


  function fireStreamAdded(info) {
    var stream = createRemoteStream(info);
    remoteStreams.set(stream.id, stream);
    var streamEvent = new StreamModule.StreamEvent('streamadded', {
      stream: stream
    });
    self.dispatchEvent(streamEvent);
  } // eslint-disable-next-line require-jsdoc


  function fireStreamRemoved(info) {
    if (!remoteStreams.has(info.id)) {
      _logger["default"].warning('Cannot find specific remote stream.');

      return;
    }

    var stream = remoteStreams.get(info.id);
    var streamEvent = new EventModule.OwtEvent('ended');
    remoteStreams["delete"](stream.id);
    stream.dispatchEvent(streamEvent);
  } // eslint-disable-next-line require-jsdoc


  function fireActiveAudioInputChange(info) {
    if (!remoteStreams.has(info.id)) {
      _logger["default"].warning('Cannot find specific remote stream.');

      return;
    }

    var stream = remoteStreams.get(info.id);
    var streamEvent = new _mixedstream.ActiveAudioInputChangeEvent('activeaudioinputchange', {
      activeAudioInputStreamId: info.data.value
    });
    stream.dispatchEvent(streamEvent);
  } // eslint-disable-next-line require-jsdoc


  function fireLayoutChange(info) {
    if (!remoteStreams.has(info.id)) {
      _logger["default"].warning('Cannot find specific remote stream.');

      return;
    }

    var stream = remoteStreams.get(info.id);
    var streamEvent = new _mixedstream.LayoutChangeEvent('layoutchange', {
      layout: info.data.value
    });
    stream.dispatchEvent(streamEvent);
  } // eslint-disable-next-line require-jsdoc


  function updateRemoteStream(streamInfo) {
    if (!remoteStreams.has(streamInfo.id)) {
      _logger["default"].warning('Cannot find specific remote stream.');

      return;
    }

    var stream = remoteStreams.get(streamInfo.id);
    stream.settings = StreamUtilsModule.convertToPublicationSettings(streamInfo.media);
    stream.extraCapabilities = StreamUtilsModule.convertToSubscriptionCapabilities(streamInfo.media);
    var streamEvent = new EventModule.OwtEvent('updated');
    stream.dispatchEvent(streamEvent);
  } // eslint-disable-next-line require-jsdoc


  function createRemoteStream(streamInfo) {
    if (streamInfo.type === 'mixed') {
      return new _mixedstream.RemoteMixedStream(streamInfo);
    } else {
      var audioSourceInfo;
      var videoSourceInfo;
      var audioTrack = streamInfo.media.tracks.find(function (t) {
        return t.type === 'audio';
      });
      var videoTrack = streamInfo.media.tracks.find(function (t) {
        return t.type === 'video';
      });

      if (audioTrack) {
        audioSourceInfo = audioTrack.source;
      }

      if (videoTrack) {
        videoSourceInfo = videoTrack.source;
      }

      var dataSourceInfo = streamInfo.data;
      var stream = new StreamModule.RemoteStream(streamInfo.id, streamInfo.info.owner, undefined, new StreamModule.StreamSourceInfo(audioSourceInfo, videoSourceInfo, dataSourceInfo), streamInfo.info.attributes);
      stream.settings = StreamUtilsModule.convertToPublicationSettings(streamInfo.media);
      stream.extraCapabilities = StreamUtilsModule.convertToSubscriptionCapabilities(streamInfo.media);
      return stream;
    }
  } // eslint-disable-next-line require-jsdoc


  function sendSignalingMessage(type, message) {
    return signaling.send(type, message);
  } // eslint-disable-next-line require-jsdoc


  function createSignalingForChannel() {
    // Construct an signaling sender/receiver for ConferencePeerConnection.
    var signalingForChannel = Object.create(EventModule.EventDispatcher);
    signalingForChannel.sendSignalingMessage = sendSignalingMessage;
    return signalingForChannel;
  } // eslint-disable-next-line require-jsdoc


  function createPeerConnectionChannel(transport) {
    var signalingForChannel = createSignalingForChannel();
    var channel = new _channel.ConferencePeerConnectionChannel(config, signalingForChannel);
    channel.addEventListener('id', function (messageEvent) {
      if (!channels.has(messageEvent.message)) {
        channels.set(messageEvent.message, channel);
      } else {
        _logger["default"].warning('Channel already exists', messageEvent.message);
      }
    });
    return channel;
  } // eslint-disable-next-line require-jsdoc


  function clean() {
    participants.clear();
    remoteStreams.clear();
  }

  Object.defineProperty(this, 'info', {
    configurable: false,
    get: function get() {
      if (!room) {
        return null;
      }

      return new _info.ConferenceInfo(room.id, Array.from(participants, function (x) {
        return x[1];
      }), Array.from(remoteStreams, function (x) {
        return x[1];
      }), me);
    }
  });
  /**
   * @function join
   * @instance
   * @desc Join a conference.
   * @memberof Owt.Conference.ConferenceClient
   * @return {Promise<ConferenceInfo, Error>} Return a promise resolved with current conference's information if successfully join the conference. Or return a promise rejected with a newly created Owt.Error if failed to join the conference.
   * @param {string} tokenString Token is issued by conference server(nuve).
   */

  this.join = function (tokenString) {
    return new Promise(function (resolve, reject) {
      var token = JSON.parse(_base.Base64.decodeBase64(tokenString));
      var isSecured = token.secure === true;
      var host = token.host;

      if (typeof host !== 'string') {
        reject(new _error.ConferenceError('Invalid host.'));
        return;
      }

      if (host.indexOf('http') === -1) {
        host = isSecured ? 'https://' + host : 'http://' + host;
      }

      if (signalingState !== SignalingState.READY) {
        reject(new _error.ConferenceError('connection state invalid'));
        return;
      }

      signalingState = SignalingState.CONNECTING;
      var sysInfo = Utils.sysInfo();
      var loginInfo = {
        token: tokenString,
        userAgent: {
          sdk: sysInfo.sdk
        },
        protocol: protocolVersion
      };
      signaling.connect(host, isSecured, loginInfo).then(function (resp) {
        signalingState = SignalingState.CONNECTED;
        room = resp.room;

        if (room.streams !== undefined) {
          var _iterator = _createForOfIteratorHelper(room.streams),
              _step;

          try {
            for (_iterator.s(); !(_step = _iterator.n()).done;) {
              var st = _step.value;

              if (st.type === 'mixed') {
                st.viewport = st.info.label;
              }

              remoteStreams.set(st.id, createRemoteStream(st));
            }
          } catch (err) {
            _iterator.e(err);
          } finally {
            _iterator.f();
          }
        }

        if (resp.room && resp.room.participants !== undefined) {
          var _iterator2 = _createForOfIteratorHelper(resp.room.participants),
              _step2;

          try {
            for (_iterator2.s(); !(_step2 = _iterator2.n()).done;) {
              var p = _step2.value;
              participants.set(p.id, new _participant2.Participant(p.id, p.role, p.user));

              if (p.id === resp.id) {
                me = participants.get(p.id);
              }
            }
          } catch (err) {
            _iterator2.e(err);
          } finally {
            _iterator2.f();
          }
        }

        peerConnectionChannel = createPeerConnectionChannel();
        peerConnectionChannel.addEventListener('ended', function () {
          peerConnectionChannel = null;
        });

        if (typeof WebTransport === 'function' && token.webTransportUrl) {
          quicTransportChannel = new _quicconnection.QuicConnection('https://jianjunz-nuc-ubuntu.sh.intel.com:7700', resp.webTransportToken, createSignalingForChannel(), config.webTransportConfiguration);
        }

        var conferenceInfo = new _info.ConferenceInfo(resp.room.id, Array.from(participants.values()), Array.from(remoteStreams.values()), me);

        if (quicTransportChannel) {
          quicTransportChannel.init().then(function () {
            resolve(conferenceInfo);
          });
        } else {
          resolve(conferenceInfo);
        }
      }, function (e) {
        signalingState = SignalingState.READY;
        reject(new _error.ConferenceError(e));
      });
    });
  };
  /**
   * @function publish
   * @memberof Owt.Conference.ConferenceClient
   * @instance
   * @desc Publish a LocalStream to conference server. Other participants will be able to subscribe this stream when it is successfully published.
   * @param {Owt.Base.LocalStream} stream The stream to be published.
   * @param {(Owt.Base.PublishOptions|RTCRtpTransceiver[])} options If options is a PublishOptions, the stream will be published as options specified. If options is a list of RTCRtpTransceivers, each track in the first argument must have a corresponding RTCRtpTransceiver here, and the track will be published with the RTCRtpTransceiver associated with it.
   * @param {string[]} videoCodecs Video codec names for publishing. Valid values are 'VP8', 'VP9' and 'H264'. This parameter only valid when the second argument is PublishOptions and options.video is RTCRtpEncodingParameters. Publishing with RTCRtpEncodingParameters is an experimental feature. This parameter is subject to change.
   * @return {Promise<Publication, Error>} Returned promise will be resolved with a newly created Publication once specific stream is successfully published, or rejected with a newly created Error if stream is invalid or options cannot be satisfied. Successfully published means PeerConnection is established and server is able to process media data.
   */


  this.publish = function (stream, options, videoCodecs) {
    if (!(stream instanceof StreamModule.LocalStream)) {
      return Promise.reject(new _error.ConferenceError('Invalid stream.'));
    }

    if (stream.source.data) {
      return quicTransportChannel.publish(stream);
    }

    if (publishChannels.has(stream.mediaStream.id)) {
      return Promise.reject(new _error.ConferenceError('Cannot publish a published stream.'));
    }

    return peerConnectionChannel.publish(stream, options, videoCodecs);
  };
  /**
   * @function subscribe
   * @memberof Owt.Conference.ConferenceClient
   * @instance
   * @desc Subscribe a RemoteStream from conference server.
   * @param {Owt.Base.RemoteStream} stream The stream to be subscribed.
   * @param {Owt.Conference.SubscribeOptions} options Options for subscription.
   * @return {Promise<Subscription, Error>} Returned promise will be resolved with a newly created Subscription once specific stream is successfully subscribed, or rejected with a newly created Error if stream is invalid or options cannot be satisfied. Successfully subscribed means PeerConnection is established and server was started to send media data.
   */


  this.subscribe = function (stream, options) {
    if (!(stream instanceof StreamModule.RemoteStream)) {
      return Promise.reject(new _error.ConferenceError('Invalid stream.'));
    }

    if (stream.source.data) {
      if (stream.source.audio || stream.source.video) {
        return Promise.reject(new TypeError('Invalid source info. A remote stream is either a data stream or ' + 'a media stream.'));
      }

      if (quicTransportChannel) {
        return quicTransportChannel.subscribe(stream);
      } else {
        return Promise.reject(new TypeError('WebTransport is not supported.'));
      }
    }

    return peerConnectionChannel.subscribe(stream, options);
  };
  /**
   * @function send
   * @memberof Owt.Conference.ConferenceClient
   * @instance
   * @desc Send a text message to a participant or all participants.
   * @param {string} message Message to be sent.
   * @param {string} participantId Receiver of this message. Message will be sent to all participants if participantId is undefined.
   * @return {Promise<void, Error>} Returned promise will be resolved when conference server received certain message.
   */


  this.send = function (message, participantId) {
    if (participantId === undefined) {
      participantId = 'all';
    }

    return sendSignalingMessage('text', {
      to: participantId,
      message: message
    });
  };
  /**
   * @function leave
   * @memberOf Owt.Conference.ConferenceClient
   * @instance
   * @desc Leave a conference.
   * @return {Promise<void, Error>} Returned promise will be resolved with undefined once the connection is disconnected.
   */


  this.leave = function () {
    return signaling.disconnect().then(function () {
      clean();
      signalingState = SignalingState.READY;
    });
  };
  /**
   * @function createSendStream
   * @memberOf Owt.Conference.ConferenceClient
   * @instance
   * @desc Create an outgoing stream. Only available when WebTransport is supported by user's browser.
   * @return {Promise<SendStream, Error>} Returned promise will be resolved with a SendStream once stream is created.
   */


  if (_quicconnection.QuicConnection) {
    this.createSendStream = /*#__PURE__*/(0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee() {
      return _regenerator["default"].wrap(function _callee$(_context) {
        while (1) {
          switch (_context.prev = _context.next) {
            case 0:
              if (quicTransportChannel) {
                _context.next = 2;
                break;
              }

              throw new _error.ConferenceError('No QUIC connection available.');

            case 2:
              return _context.abrupt("return", quicTransportChannel.createSendStream());

            case 3:
            case "end":
              return _context.stop();
          }
        }
      }, _callee);
    }));
  }
};

exports.ConferenceClient = ConferenceClient;

},{"../base/base64.js":24,"../base/event.js":26,"../base/logger.js":28,"../base/stream.js":33,"../base/utils.js":35,"./channel.js":36,"./error.js":38,"./info.js":40,"./mixedstream.js":41,"./participant.js":42,"./quicconnection.js":43,"./signaling.js":44,"./streamutils.js":45,"@babel/runtime/helpers/asyncToGenerator":4,"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/interopRequireWildcard":11,"@babel/runtime/regenerator":22}],38:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';
/**
 * @class ConferenceError
 * @classDesc The ConferenceError object represents an error in conference mode.
 * @memberOf Owt.Conference
 * @hideconstructor
 */

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.ConferenceError = void 0;

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var _inherits2 = _interopRequireDefault(require("@babel/runtime/helpers/inherits"));

var _possibleConstructorReturn2 = _interopRequireDefault(require("@babel/runtime/helpers/possibleConstructorReturn"));

var _getPrototypeOf2 = _interopRequireDefault(require("@babel/runtime/helpers/getPrototypeOf"));

var _wrapNativeSuper2 = _interopRequireDefault(require("@babel/runtime/helpers/wrapNativeSuper"));

function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = (0, _getPrototypeOf2["default"])(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return (0, _possibleConstructorReturn2["default"])(this, result); }; }

function _isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

var ConferenceError = /*#__PURE__*/function (_Error) {
  (0, _inherits2["default"])(ConferenceError, _Error);

  var _super = _createSuper(ConferenceError);

  // eslint-disable-next-line require-jsdoc
  function ConferenceError(message) {
    (0, _classCallCheck2["default"])(this, ConferenceError);
    return _super.call(this, message);
  }

  return ConferenceError;
}( /*#__PURE__*/(0, _wrapNativeSuper2["default"])(Error));

exports.ConferenceError = ConferenceError;

},{"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/getPrototypeOf":8,"@babel/runtime/helpers/inherits":9,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/possibleConstructorReturn":16,"@babel/runtime/helpers/wrapNativeSuper":21}],39:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
Object.defineProperty(exports, "ConferenceClient", {
  enumerable: true,
  get: function get() {
    return _client.ConferenceClient;
  }
});
Object.defineProperty(exports, "SioSignaling", {
  enumerable: true,
  get: function get() {
    return _signaling.SioSignaling;
  }
});

var _client = require("./client.js");

var _signaling = require("./signaling.js");

},{"./client.js":37,"./signaling.js":44}],40:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';
/**
 * @class ConferenceInfo
 * @classDesc Information for a conference.
 * @memberOf Owt.Conference
 * @hideconstructor
 */

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.ConferenceInfo = void 0;

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var ConferenceInfo = // eslint-disable-next-line require-jsdoc
function ConferenceInfo(id, participants, remoteStreams, myInfo) {
  (0, _classCallCheck2["default"])(this, ConferenceInfo);

  /**
   * @member {string} id
   * @instance
   * @memberof Owt.Conference.ConferenceInfo
   * @desc Conference ID.
   */
  this.id = id;
  /**
   * @member {Array<Owt.Conference.Participant>} participants
   * @instance
   * @memberof Owt.Conference.ConferenceInfo
   * @desc Participants in the conference.
   */

  this.participants = participants;
  /**
   * @member {Array<Owt.Base.RemoteStream>} remoteStreams
   * @instance
   * @memberof Owt.Conference.ConferenceInfo
   * @desc Streams published by participants. It also includes streams published by current user.
   */

  this.remoteStreams = remoteStreams;
  /**
   * @member {Owt.Base.Participant} self
   * @instance
   * @memberof Owt.Conference.ConferenceInfo
   */

  this.self = myInfo;
};

exports.ConferenceInfo = ConferenceInfo;

},{"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/interopRequireDefault":10}],41:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';

var _interopRequireWildcard = require("@babel/runtime/helpers/interopRequireWildcard");

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.LayoutChangeEvent = exports.ActiveAudioInputChangeEvent = exports.RemoteMixedStream = void 0;

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var _inherits2 = _interopRequireDefault(require("@babel/runtime/helpers/inherits"));

var _possibleConstructorReturn2 = _interopRequireDefault(require("@babel/runtime/helpers/possibleConstructorReturn"));

var _getPrototypeOf2 = _interopRequireDefault(require("@babel/runtime/helpers/getPrototypeOf"));

var StreamModule = _interopRequireWildcard(require("../base/stream.js"));

var StreamUtilsModule = _interopRequireWildcard(require("./streamutils.js"));

var _event = require("../base/event.js");

function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = (0, _getPrototypeOf2["default"])(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return (0, _possibleConstructorReturn2["default"])(this, result); }; }

function _isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

/**
 * @class RemoteMixedStream
 * @classDesc Mixed stream from conference server.
 * Events:
 *
 * | Event Name             | Argument Type    | Fired when       |
 * | -----------------------| ---------------- | ---------------- |
 * | activeaudioinputchange | Event            | Audio activeness of input stream (of the mixed stream) is changed. |
 * | layoutchange           | Event            | Video's layout has been changed. It usually happens when a new video is mixed into the target mixed stream or an existing video has been removed from mixed stream. |
 *
 * @memberOf Owt.Conference
 * @extends Owt.Base.RemoteStream
 * @hideconstructor
 */
var RemoteMixedStream = /*#__PURE__*/function (_StreamModule$RemoteS) {
  (0, _inherits2["default"])(RemoteMixedStream, _StreamModule$RemoteS);

  var _super = _createSuper(RemoteMixedStream);

  // eslint-disable-next-line require-jsdoc
  function RemoteMixedStream(info) {
    var _this;

    (0, _classCallCheck2["default"])(this, RemoteMixedStream);

    if (info.type !== 'mixed') {
      throw new TypeError('Not a mixed stream');
    }

    _this = _super.call(this, info.id, undefined, undefined, new StreamModule.StreamSourceInfo('mixed', 'mixed'));
    _this.settings = StreamUtilsModule.convertToPublicationSettings(info.media);
    _this.extraCapabilities = StreamUtilsModule.convertToSubscriptionCapabilities(info.media);
    return _this;
  }

  return RemoteMixedStream;
}(StreamModule.RemoteStream);
/**
 * @class ActiveAudioInputChangeEvent
 * @classDesc Class ActiveAudioInputChangeEvent represents an active audio input change event.
 * @memberof Owt.Conference
 * @hideconstructor
 */


exports.RemoteMixedStream = RemoteMixedStream;

var ActiveAudioInputChangeEvent = /*#__PURE__*/function (_OwtEvent) {
  (0, _inherits2["default"])(ActiveAudioInputChangeEvent, _OwtEvent);

  var _super2 = _createSuper(ActiveAudioInputChangeEvent);

  // eslint-disable-next-line require-jsdoc
  function ActiveAudioInputChangeEvent(type, init) {
    var _this2;

    (0, _classCallCheck2["default"])(this, ActiveAudioInputChangeEvent);
    _this2 = _super2.call(this, type);
    /**
     * @member {string} activeAudioInputStreamId
     * @instance
     * @memberof Owt.Conference.ActiveAudioInputChangeEvent
     * @desc The ID of input stream(of the mixed stream) whose audio is active.
     */

    _this2.activeAudioInputStreamId = init.activeAudioInputStreamId;
    return _this2;
  }

  return ActiveAudioInputChangeEvent;
}(_event.OwtEvent);
/**
 * @class LayoutChangeEvent
 * @classDesc Class LayoutChangeEvent represents an video layout change event.
 * @memberof Owt.Conference
 * @hideconstructor
 */


exports.ActiveAudioInputChangeEvent = ActiveAudioInputChangeEvent;

var LayoutChangeEvent = /*#__PURE__*/function (_OwtEvent2) {
  (0, _inherits2["default"])(LayoutChangeEvent, _OwtEvent2);

  var _super3 = _createSuper(LayoutChangeEvent);

  // eslint-disable-next-line require-jsdoc
  function LayoutChangeEvent(type, init) {
    var _this3;

    (0, _classCallCheck2["default"])(this, LayoutChangeEvent);
    _this3 = _super3.call(this, type);
    /**
     * @member {object} layout
     * @instance
     * @memberof Owt.Conference.LayoutChangeEvent
     * @desc Current video's layout. It's an array of map which maps each stream to a region.
     */

    _this3.layout = init.layout;
    return _this3;
  }

  return LayoutChangeEvent;
}(_event.OwtEvent);

exports.LayoutChangeEvent = LayoutChangeEvent;

},{"../base/event.js":26,"../base/stream.js":33,"./streamutils.js":45,"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/getPrototypeOf":8,"@babel/runtime/helpers/inherits":9,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/interopRequireWildcard":11,"@babel/runtime/helpers/possibleConstructorReturn":16}],42:[function(require,module,exports){
"use strict";

var _interopRequireWildcard = require("@babel/runtime/helpers/interopRequireWildcard");

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Participant = void 0;

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var _assertThisInitialized2 = _interopRequireDefault(require("@babel/runtime/helpers/assertThisInitialized"));

var _inherits2 = _interopRequireDefault(require("@babel/runtime/helpers/inherits"));

var _possibleConstructorReturn2 = _interopRequireDefault(require("@babel/runtime/helpers/possibleConstructorReturn"));

var _getPrototypeOf2 = _interopRequireDefault(require("@babel/runtime/helpers/getPrototypeOf"));

var EventModule = _interopRequireWildcard(require("../base/event.js"));

function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = (0, _getPrototypeOf2["default"])(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return (0, _possibleConstructorReturn2["default"])(this, result); }; }

function _isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

'use strict';
/**
 * @class Participant
 * @memberOf Owt.Conference
 * @classDesc The Participant defines a participant in a conference.
 * Events:
 *
 * | Event Name      | Argument Type      | Fired when       |
 * | ----------------| ------------------ | ---------------- |
 * | left            | Owt.Base.OwtEvent  | The participant left the conference. |
 *
 * @extends Owt.Base.EventDispatcher
 * @hideconstructor
 */


var Participant = /*#__PURE__*/function (_EventModule$EventDis) {
  (0, _inherits2["default"])(Participant, _EventModule$EventDis);

  var _super = _createSuper(Participant);

  // eslint-disable-next-line require-jsdoc
  function Participant(id, role, userId) {
    var _this;

    (0, _classCallCheck2["default"])(this, Participant);
    _this = _super.call(this);
    /**
     * @member {string} id
     * @instance
     * @memberof Owt.Conference.Participant
     * @desc The ID of the participant. It varies when a single user join different conferences.
     */

    Object.defineProperty((0, _assertThisInitialized2["default"])(_this), 'id', {
      configurable: false,
      writable: false,
      value: id
    });
    /**
     * @member {string} role
     * @instance
     * @memberof Owt.Conference.Participant
     */

    Object.defineProperty((0, _assertThisInitialized2["default"])(_this), 'role', {
      configurable: false,
      writable: false,
      value: role
    });
    /**
     * @member {string} userId
     * @instance
     * @memberof Owt.Conference.Participant
     * @desc The user ID of the participant. It can be integrated into existing account management system.
     */

    Object.defineProperty((0, _assertThisInitialized2["default"])(_this), 'userId', {
      configurable: false,
      writable: false,
      value: userId
    });
    return _this;
  }

  return Participant;
}(EventModule.EventDispatcher);

exports.Participant = Participant;

},{"../base/event.js":26,"@babel/runtime/helpers/assertThisInitialized":3,"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/getPrototypeOf":8,"@babel/runtime/helpers/inherits":9,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/interopRequireWildcard":11,"@babel/runtime/helpers/possibleConstructorReturn":16}],43:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/* eslint-disable require-jsdoc */

/* global Promise, Map, WebTransport, Uint8Array, Uint32Array, TextEncoder */
'use strict';

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.QuicConnection = void 0;

var _regenerator = _interopRequireDefault(require("@babel/runtime/regenerator"));

var _asyncToGenerator2 = _interopRequireDefault(require("@babel/runtime/helpers/asyncToGenerator"));

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var _createClass2 = _interopRequireDefault(require("@babel/runtime/helpers/createClass"));

var _inherits2 = _interopRequireDefault(require("@babel/runtime/helpers/inherits"));

var _possibleConstructorReturn2 = _interopRequireDefault(require("@babel/runtime/helpers/possibleConstructorReturn"));

var _getPrototypeOf2 = _interopRequireDefault(require("@babel/runtime/helpers/getPrototypeOf"));

var _logger = _interopRequireDefault(require("../base/logger.js"));

var _event = require("../base/event.js");

var _publication = require("../base/publication.js");

var _subscription = require("./subscription.js");

var _base = require("../base/base64.js");

function _createForOfIteratorHelper(o, allowArrayLike) { var it; if (typeof Symbol === "undefined" || o[Symbol.iterator] == null) { if (Array.isArray(o) || (it = _unsupportedIterableToArray(o)) || allowArrayLike && o && typeof o.length === "number") { if (it) o = it; var i = 0; var F = function F() {}; return { s: F, n: function n() { if (i >= o.length) return { done: true }; return { done: false, value: o[i++] }; }, e: function e(_e) { throw _e; }, f: F }; } throw new TypeError("Invalid attempt to iterate non-iterable instance.\nIn order to be iterable, non-array objects must have a [Symbol.iterator]() method."); } var normalCompletion = true, didErr = false, err; return { s: function s() { it = o[Symbol.iterator](); }, n: function n() { var step = it.next(); normalCompletion = step.done; return step; }, e: function e(_e2) { didErr = true; err = _e2; }, f: function f() { try { if (!normalCompletion && it["return"] != null) it["return"](); } finally { if (didErr) throw err; } } }; }

function _unsupportedIterableToArray(o, minLen) { if (!o) return; if (typeof o === "string") return _arrayLikeToArray(o, minLen); var n = Object.prototype.toString.call(o).slice(8, -1); if (n === "Object" && o.constructor) n = o.constructor.name; if (n === "Map" || n === "Set") return Array.from(o); if (n === "Arguments" || /^(?:Ui|I)nt(?:8|16|32)(?:Clamped)?Array$/.test(n)) return _arrayLikeToArray(o, minLen); }

function _arrayLikeToArray(arr, len) { if (len == null || len > arr.length) len = arr.length; for (var i = 0, arr2 = new Array(len); i < len; i++) { arr2[i] = arr[i]; } return arr2; }

function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = (0, _getPrototypeOf2["default"])(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return (0, _possibleConstructorReturn2["default"])(this, result); }; }

function _isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

/**
 * @class QuicConnection
 * @classDesc A channel for a QUIC transport between client and conference
 * server.
 * @hideconstructor
 * @private
 */
var QuicConnection = /*#__PURE__*/function (_EventDispatcher) {
  (0, _inherits2["default"])(QuicConnection, _EventDispatcher);

  var _super = _createSuper(QuicConnection);

  // `tokenString` is a base64 string of the token object. It's in the return
  // value of `ConferenceClient.join`.
  function QuicConnection(url, tokenString, signaling, webTransportOptions) {
    var _this;

    (0, _classCallCheck2["default"])(this, QuicConnection);
    _this = _super.call(this);
    _this._tokenString = tokenString;
    _this._token = JSON.parse(_base.Base64.decodeBase64(tokenString));
    _this._signaling = signaling;
    _this._ended = false;
    _this._quicStreams = new Map(); // Key is publication or subscription ID.

    _this._quicTransport = new WebTransport(url, webTransportOptions);
    _this._subscribePromises = new Map(); // Key is subscription ID.

    _this._transportId = _this._token.transportId;

    _this._initReceiveStreamReader();

    return _this;
  }
  /**
   * @function onMessage
   * @desc Received a message from conference portal. Defined in client-server
   * protocol.
   * @param {string} notification Notification type.
   * @param {object} message Message received.
   * @private
   */


  (0, _createClass2["default"])(QuicConnection, [{
    key: "onMessage",
    value: function onMessage(notification, message) {
      switch (notification) {
        case 'progress':
          if (message.status === 'soac') {
            this._soacHandler(message.data);
          } else if (message.status === 'ready') {
            this._readyHandler();
          } else if (message.status === 'error') {
            this._errorHandler(message.data);
          }

          break;

        case 'stream':
          this._onStreamEvent(message);

          break;

        default:
          _logger["default"].warning('Unknown notification from MCU.');

      }
    }
  }, {
    key: "init",
    value: function () {
      var _init = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee() {
        return _regenerator["default"].wrap(function _callee$(_context) {
          while (1) {
            switch (_context.prev = _context.next) {
              case 0:
                _context.next = 2;
                return this._authenticate(this._tokenString);

              case 2:
              case "end":
                return _context.stop();
            }
          }
        }, _callee, this);
      }));

      function init() {
        return _init.apply(this, arguments);
      }

      return init;
    }()
  }, {
    key: "_initReceiveStreamReader",
    value: function () {
      var _initReceiveStreamReader2 = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee2() {
        var receiveStreamReader, receivingDone, _yield$receiveStreamR, receiveStream, readingReceiveStreamsDone, chunkReader, _yield$chunkReader$re, uuid, readingChunksDone, subscriptionId, subscription;

        return _regenerator["default"].wrap(function _callee2$(_context2) {
          while (1) {
            switch (_context2.prev = _context2.next) {
              case 0:
                receiveStreamReader = this._quicTransport.incomingBidirectionalStreams.getReader();

                _logger["default"].info('Reader: ' + receiveStreamReader);

                receivingDone = false;

              case 3:
                if (receivingDone) {
                  _context2.next = 31;
                  break;
                }

                _context2.next = 6;
                return receiveStreamReader.read();

              case 6:
                _yield$receiveStreamR = _context2.sent;
                receiveStream = _yield$receiveStreamR.value;
                readingReceiveStreamsDone = _yield$receiveStreamR.done;

                _logger["default"].info('New stream received');

                if (!readingReceiveStreamsDone) {
                  _context2.next = 13;
                  break;
                }

                receivingDone = true;
                return _context2.abrupt("break", 31);

              case 13:
                chunkReader = receiveStream.readable.getReader();
                _context2.next = 16;
                return chunkReader.read();

              case 16:
                _yield$chunkReader$re = _context2.sent;
                uuid = _yield$chunkReader$re.value;
                readingChunksDone = _yield$chunkReader$re.done;

                if (!readingChunksDone) {
                  _context2.next = 22;
                  break;
                }

                _logger["default"].error('Stream closed unexpectedly.');

                return _context2.abrupt("return");

              case 22:
                if (!(uuid.length != 16)) {
                  _context2.next = 25;
                  break;
                }

                _logger["default"].error('Unexpected length for UUID.');

                return _context2.abrupt("return");

              case 25:
                chunkReader.releaseLock();
                subscriptionId = this._uint8ArrayToUuid(uuid);

                this._quicStreams.set(subscriptionId, receiveStream);

                if (this._subscribePromises.has(subscriptionId)) {
                  subscription = this._createSubscription(subscriptionId, receiveStream);

                  this._subscribePromises.get(subscriptionId).resolve(subscription);
                }

                _context2.next = 3;
                break;

              case 31:
              case "end":
                return _context2.stop();
            }
          }
        }, _callee2, this);
      }));

      function _initReceiveStreamReader() {
        return _initReceiveStreamReader2.apply(this, arguments);
      }

      return _initReceiveStreamReader;
    }()
  }, {
    key: "_createSubscription",
    value: function _createSubscription(id, receiveStream) {
      // TODO: Incomplete subscription.
      var subscription = new _subscription.Subscription(id, function () {
        receiveStream.abortReading();
      });
      subscription.stream = receiveStream;
      return subscription;
    }
  }, {
    key: "_authenticate",
    value: function () {
      var _authenticate2 = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee3(token) {
        var quicStream, chunkReader, writer, encoder, encodedToken;
        return _regenerator["default"].wrap(function _callee3$(_context3) {
          while (1) {
            switch (_context3.prev = _context3.next) {
              case 0:
                _context3.next = 2;
                return this._quicTransport.ready;

              case 2:
                _context3.next = 4;
                return this._quicTransport.createBidirectionalStream();

              case 4:
                quicStream = _context3.sent;
                chunkReader = quicStream.readable.getReader();
                writer = quicStream.writable.getWriter();
                _context3.next = 9;
                return writer.ready;

              case 9:
                // 128 bit of zero indicates this is a stream for signaling.
                writer.write(new Uint8Array(16)); // Send token as described in
                // https://github.com/open-webrtc-toolkit/owt-server/blob/20e8aad216cc446095f63c409339c34c7ee770ee/doc/design/quic-transport-payload-format.md.

                encoder = new TextEncoder();
                encodedToken = encoder.encode(token);
                writer.write(Uint32Array.of(encodedToken.length));
                writer.write(encodedToken); // Server returns transport ID as an ack. Ignore it here.

                _context3.next = 16;
                return chunkReader.read();

              case 16:
                _logger["default"].info('Authentication success.');

              case 17:
              case "end":
                return _context3.stop();
            }
          }
        }, _callee3, this);
      }));

      function _authenticate(_x) {
        return _authenticate2.apply(this, arguments);
      }

      return _authenticate;
    }()
  }, {
    key: "createSendStream",
    value: function () {
      var _createSendStream = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee4() {
        var quicStream;
        return _regenerator["default"].wrap(function _callee4$(_context4) {
          while (1) {
            switch (_context4.prev = _context4.next) {
              case 0:
                _context4.next = 2;
                return this._quicTransport.ready;

              case 2:
                _context4.next = 4;
                return this._quicTransport.createBidirectionalStream();

              case 4:
                quicStream = _context4.sent;
                return _context4.abrupt("return", quicStream);

              case 6:
              case "end":
                return _context4.stop();
            }
          }
        }, _callee4, this);
      }));

      function createSendStream() {
        return _createSendStream.apply(this, arguments);
      }

      return createSendStream;
    }()
  }, {
    key: "createSendStream1",
    value: function () {
      var _createSendStream2 = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee5(sessionId) {
        var publicationId, quicStream, writer;
        return _regenerator["default"].wrap(function _callee5$(_context5) {
          while (1) {
            switch (_context5.prev = _context5.next) {
              case 0:
                _logger["default"].info('Create stream.');

                _context5.next = 3;
                return this._quicTransport.ready;

              case 3:
                _context5.next = 5;
                return this._initiatePublication();

              case 5:
                publicationId = _context5.sent;
                _context5.next = 8;
                return this._quicTransport.createSendStream();

              case 8:
                quicStream = _context5.sent;
                writer = quicStream.writable.getWriter();
                _context5.next = 12;
                return writer.ready;

              case 12:
                writer.write(this._uuidToUint8Array(publicationId));
                writer.releaseLock();

                this._quicStreams.set(publicationId, quicStream);

                return _context5.abrupt("return", quicStream);

              case 16:
              case "end":
                return _context5.stop();
            }
          }
        }, _callee5, this);
      }));

      function createSendStream1(_x2) {
        return _createSendStream2.apply(this, arguments);
      }

      return createSendStream1;
    }()
  }, {
    key: "publish",
    value: function () {
      var _publish = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee6(stream) {
        var _this2 = this;

        var publicationId, quicStream, writer, publication;
        return _regenerator["default"].wrap(function _callee6$(_context6) {
          while (1) {
            switch (_context6.prev = _context6.next) {
              case 0:
                _context6.next = 2;
                return this._initiatePublication();

              case 2:
                publicationId = _context6.sent;
                quicStream = stream.stream;
                writer = quicStream.writable.getWriter();
                _context6.next = 7;
                return writer.ready;

              case 7:
                writer.write(this._uuidToUint8Array(publicationId));
                writer.releaseLock();

                _logger["default"].info('publish id');

                this._quicStreams.set(publicationId, quicStream);

                publication = new _publication.Publication(publicationId, function () {
                  _this2._signaling.sendSignalingMessage('unpublish', {
                    id: publication
                  })["catch"](function (e) {
                    _logger["default"].warning('MCU returns negative ack for unpublishing, ' + e);
                  });
                }
                /* TODO: getStats, mute, unmute is not implemented */
                );
                return _context6.abrupt("return", publication);

              case 13:
              case "end":
                return _context6.stop();
            }
          }
        }, _callee6, this);
      }));

      function publish(_x3) {
        return _publish.apply(this, arguments);
      }

      return publish;
    }()
  }, {
    key: "hasContentSessionId",
    value: function hasContentSessionId(id) {
      return this._quicStreams.has(id);
    }
  }, {
    key: "_uuidToUint8Array",
    value: function _uuidToUint8Array(uuidString) {
      if (uuidString.length != 32) {
        throw new TypeError('Incorrect UUID.');
      }

      var uuidArray = new Uint8Array(16);

      for (var i = 0; i < 16; i++) {
        uuidArray[i] = parseInt(uuidString.substring(i * 2, i * 2 + 2), 16);
      }

      return uuidArray;
    }
  }, {
    key: "_uint8ArrayToUuid",
    value: function _uint8ArrayToUuid(uuidBytes) {
      var s = '';

      var _iterator = _createForOfIteratorHelper(uuidBytes),
          _step;

      try {
        for (_iterator.s(); !(_step = _iterator.n()).done;) {
          var hex = _step.value;
          var str = hex.toString(16);
          s += str.padStart(2, '0');
        }
      } catch (err) {
        _iterator.e(err);
      } finally {
        _iterator.f();
      }

      return s;
    }
  }, {
    key: "subscribe",
    value: function subscribe(stream) {
      var _this3 = this;

      var p = new Promise(function (resolve, reject) {
        _this3._signaling.sendSignalingMessage('subscribe', {
          media: null,
          data: {
            from: stream.id
          },
          transport: {
            type: 'quic',
            id: _this3._transportId
          }
        }).then(function (data) {
          if (_this3._quicStreams.has(data.id)) {
            // QUIC stream created before signaling returns.
            var subscription = _this3._createSubscription(data.id, _this3._quicStreams.get(data.id));

            resolve(subscription);
          } else {
            _this3._quicStreams.set(data.id, null); // QUIC stream is not created yet, resolve promise after getting
            // QUIC stream.


            _this3._subscribePromises.set(data.id, {
              resolve: resolve,
              reject: reject
            });
          }
        });
      });
      return p;
    }
  }, {
    key: "_readAndPrint",
    value: function _readAndPrint() {
      var _this4 = this;

      this._quicStreams[0].waitForReadable(5).then(function () {
        var data = new Uint8Array(_this4._quicStreams[0].readBufferedAmount);

        _this4._quicStreams[0].readInto(data);

        _logger["default"].info('Read data: ' + data);

        _this4._readAndPrint();
      });
    }
  }, {
    key: "_initiatePublication",
    value: function () {
      var _initiatePublication2 = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee7() {
        var data;
        return _regenerator["default"].wrap(function _callee7$(_context7) {
          while (1) {
            switch (_context7.prev = _context7.next) {
              case 0:
                _context7.next = 2;
                return this._signaling.sendSignalingMessage('publish', {
                  media: null,
                  data: true,
                  transport: {
                    type: 'quic',
                    id: this._transportId
                  }
                });

              case 2:
                data = _context7.sent;

                if (!(this._transportId !== data.transportId)) {
                  _context7.next = 5;
                  break;
                }

                throw new Error('Transport ID not match.');

              case 5:
                return _context7.abrupt("return", data.id);

              case 6:
              case "end":
                return _context7.stop();
            }
          }
        }, _callee7, this);
      }));

      function _initiatePublication() {
        return _initiatePublication2.apply(this, arguments);
      }

      return _initiatePublication;
    }()
  }, {
    key: "_readyHandler",
    value: function _readyHandler() {// Ready message from server is useless for QuicStream since QuicStream has
      // its own status. Do nothing here.
    }
  }]);
  return QuicConnection;
}(_event.EventDispatcher);

exports.QuicConnection = QuicConnection;

},{"../base/base64.js":24,"../base/event.js":26,"../base/logger.js":28,"../base/publication.js":31,"./subscription.js":46,"@babel/runtime/helpers/asyncToGenerator":4,"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/createClass":7,"@babel/runtime/helpers/getPrototypeOf":8,"@babel/runtime/helpers/inherits":9,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/possibleConstructorReturn":16,"@babel/runtime/regenerator":22}],44:[function(require,module,exports){
"use strict";

var _interopRequireWildcard = require("@babel/runtime/helpers/interopRequireWildcard");

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.SioSignaling = void 0;

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var _createClass2 = _interopRequireDefault(require("@babel/runtime/helpers/createClass"));

var _inherits2 = _interopRequireDefault(require("@babel/runtime/helpers/inherits"));

var _possibleConstructorReturn2 = _interopRequireDefault(require("@babel/runtime/helpers/possibleConstructorReturn"));

var _getPrototypeOf2 = _interopRequireDefault(require("@babel/runtime/helpers/getPrototypeOf"));

var _logger = _interopRequireDefault(require("../base/logger.js"));

var EventModule = _interopRequireWildcard(require("../base/event.js"));

var _error = require("./error.js");

var _base = require("../base/base64.js");

function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = (0, _getPrototypeOf2["default"])(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return (0, _possibleConstructorReturn2["default"])(this, result); }; }

function _isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

'use strict';

var reconnectionAttempts = 10; // eslint-disable-next-line require-jsdoc

function handleResponse(status, data, resolve, reject) {
  if (status === 'ok' || status === 'success') {
    resolve(data);
  } else if (status === 'error') {
    reject(data);
  } else {
    _logger["default"].error('MCU returns unknown ack.');
  }
}
/**
 * @class SioSignaling
 * @classdesc Socket.IO signaling channel for ConferenceClient. It is not recommended to directly access this class.
 * @memberof Owt.Conference
 * @extends Owt.Base.EventDispatcher
 * @constructor
 */


var SioSignaling = /*#__PURE__*/function (_EventModule$EventDis) {
  (0, _inherits2["default"])(SioSignaling, _EventModule$EventDis);

  var _super = _createSuper(SioSignaling);

  // eslint-disable-next-line require-jsdoc
  function SioSignaling() {
    var _this;

    (0, _classCallCheck2["default"])(this, SioSignaling);
    _this = _super.call(this);
    _this._socket = null;
    _this._loggedIn = false;
    _this._reconnectTimes = 0;
    _this._reconnectionTicket = null;
    _this._refreshReconnectionTicket = null;
    return _this;
  }
  /**
   * @function connect
   * @instance
   * @desc Connect to a portal.
   * @memberof Oms.Conference.SioSignaling
   * @return {Promise<Object, Error>} Return a promise resolved with the data returned by portal if successfully logged in. Or return a promise rejected with a newly created Oms.Error if failed.
   * @param {string} host Host of the portal.
   * @param {string} isSecured Is secure connection or not.
   * @param {string} loginInfo Information required for logging in.
   * @private.
   */


  (0, _createClass2["default"])(SioSignaling, [{
    key: "connect",
    value: function connect(host, isSecured, loginInfo) {
      var _this2 = this;

      return new Promise(function (resolve, reject) {
        var opts = {
          'reconnection': true,
          'reconnectionAttempts': reconnectionAttempts,
          'force new connection': true
        };
        _this2._socket = io(host, opts);
        ['participant', 'text', 'stream', 'progress'].forEach(function (notification) {
          _this2._socket.on(notification, function (data) {
            _this2.dispatchEvent(new EventModule.MessageEvent('data', {
              message: {
                notification: notification,
                data: data
              }
            }));
          });
        });

        _this2._socket.on('reconnecting', function () {
          _this2._reconnectTimes++;
        });

        _this2._socket.on('reconnect_failed', function () {
          if (_this2._reconnectTimes >= reconnectionAttempts) {
            _this2.dispatchEvent(new EventModule.OwtEvent('disconnect'));
          }
        });

        _this2._socket.on('connect_error', function (e) {
          reject("connect_error:".concat(host));
        });

        _this2._socket.on('drop', function () {
          _this2._reconnectTimes = reconnectionAttempts;
        });

        _this2._socket.on('disconnect', function () {
          _this2._clearReconnectionTask();

          if (_this2._reconnectTimes >= reconnectionAttempts) {
            _this2._loggedIn = false;

            _this2.dispatchEvent(new EventModule.OwtEvent('disconnect'));
          }
        });

        _this2._socket.emit('login', loginInfo, function (status, data) {
          if (status === 'ok') {
            _this2._loggedIn = true;

            _this2._onReconnectionTicket(data.reconnectionTicket);

            _this2._socket.on('connect', function () {
              // re-login with reconnection ticket.
              _this2._socket.emit('relogin', _this2._reconnectionTicket, function (status, data) {
                if (status === 'ok') {
                  _this2._reconnectTimes = 0;

                  _this2._onReconnectionTicket(data);
                } else {
                  _this2.dispatchEvent(new EventModule.OwtEvent('disconnect'));
                }
              });
            });
          }

          handleResponse(status, data, resolve, reject);
        });
      });
    }
    /**
     * @function disconnect
     * @instance
     * @desc Disconnect from a portal.
     * @memberof Oms.Conference.SioSignaling
     * @return {Promise<Object, Error>} Return a promise resolved with the data returned by portal if successfully disconnected. Or return a promise rejected with a newly created Oms.Error if failed.
     * @private.
     */

  }, {
    key: "disconnect",
    value: function disconnect() {
      var _this3 = this;

      if (!this._socket || this._socket.disconnected) {
        return Promise.reject(new _error.ConferenceError('Portal is not connected.'));
      }

      return new Promise(function (resolve, reject) {
        _this3._socket.emit('logout', function (status, data) {
          // Maximize the reconnect times to disable reconnection.
          _this3._reconnectTimes = reconnectionAttempts;

          _this3._socket.disconnect();

          handleResponse(status, data, resolve, reject);
        });
      });
    }
    /**
     * @function send
     * @instance
     * @desc Send data to portal.
     * @memberof Oms.Conference.SioSignaling
     * @return {Promise<Object, Error>} Return a promise resolved with the data returned by portal. Or return a promise rejected with a newly created Oms.Error if failed to send the message.
     * @param {string} requestName Name defined in client-server protocol.
     * @param {string} requestData Data format is defined in client-server protocol.
     * @private.
     */

  }, {
    key: "send",
    value: function send(requestName, requestData) {
      var _this4 = this;

      return new Promise(function (resolve, reject) {
        _this4._socket.emit(requestName, requestData, function (status, data) {
          handleResponse(status, data, resolve, reject);
        });
      });
    }
    /**
     * @function _onReconnectionTicket
     * @instance
     * @desc Parse reconnection ticket and schedule ticket refreshing.
     * @param {string} ticketString Reconnection ticket.
     * @memberof Owt.Conference.SioSignaling
     * @private.
     */

  }, {
    key: "_onReconnectionTicket",
    value: function _onReconnectionTicket(ticketString) {
      var _this5 = this;

      this._reconnectionTicket = ticketString;
      var ticket = JSON.parse(_base.Base64.decodeBase64(ticketString)); // Refresh ticket 1 min or 10 seconds before it expires.

      var now = Date.now();
      var millisecondsInOneMinute = 60 * 1000;
      var millisecondsInTenSeconds = 10 * 1000;

      if (ticket.notAfter <= now - millisecondsInTenSeconds) {
        _logger["default"].warning('Reconnection ticket expires too soon.');

        return;
      }

      var refreshAfter = ticket.notAfter - now - millisecondsInOneMinute;

      if (refreshAfter < 0) {
        refreshAfter = ticket.notAfter - now - millisecondsInTenSeconds;
      }

      this._clearReconnectionTask();

      this._refreshReconnectionTicket = setTimeout(function () {
        _this5._socket.emit('refreshReconnectionTicket', function (status, data) {
          if (status !== 'ok') {
            _logger["default"].warning('Failed to refresh reconnection ticket.');

            return;
          }

          _this5._onReconnectionTicket(data);
        });
      }, refreshAfter);
    }
    /**
     * @function _clearReconnectionTask
     * @instance
     * @desc Stop trying to refresh reconnection ticket.
     * @memberof Owt.Conference.SioSignaling
     * @private.
     */

  }, {
    key: "_clearReconnectionTask",
    value: function _clearReconnectionTask() {
      clearTimeout(this._refreshReconnectionTicket);
      this._refreshReconnectionTicket = null;
    }
  }]);
  return SioSignaling;
}(EventModule.EventDispatcher);

exports.SioSignaling = SioSignaling;

},{"../base/base64.js":24,"../base/event.js":26,"../base/logger.js":28,"./error.js":38,"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/createClass":7,"@babel/runtime/helpers/getPrototypeOf":8,"@babel/runtime/helpers/inherits":9,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/interopRequireWildcard":11,"@babel/runtime/helpers/possibleConstructorReturn":16}],45:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// This file doesn't have public APIs.

/* eslint-disable valid-jsdoc */
'use strict';

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

var _interopRequireWildcard = require("@babel/runtime/helpers/interopRequireWildcard");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.convertToPublicationSettings = convertToPublicationSettings;
exports.convertToSubscriptionCapabilities = convertToSubscriptionCapabilities;

var PublicationModule = _interopRequireWildcard(require("../base/publication.js"));

var MediaFormatModule = _interopRequireWildcard(require("../base/mediaformat.js"));

var CodecModule = _interopRequireWildcard(require("../base/codec.js"));

var SubscriptionModule = _interopRequireWildcard(require("./subscription.js"));

var _logger = _interopRequireDefault(require("../base/logger.js"));

function _createForOfIteratorHelper(o, allowArrayLike) { var it; if (typeof Symbol === "undefined" || o[Symbol.iterator] == null) { if (Array.isArray(o) || (it = _unsupportedIterableToArray(o)) || allowArrayLike && o && typeof o.length === "number") { if (it) o = it; var i = 0; var F = function F() {}; return { s: F, n: function n() { if (i >= o.length) return { done: true }; return { done: false, value: o[i++] }; }, e: function e(_e) { throw _e; }, f: F }; } throw new TypeError("Invalid attempt to iterate non-iterable instance.\nIn order to be iterable, non-array objects must have a [Symbol.iterator]() method."); } var normalCompletion = true, didErr = false, err; return { s: function s() { it = o[Symbol.iterator](); }, n: function n() { var step = it.next(); normalCompletion = step.done; return step; }, e: function e(_e2) { didErr = true; err = _e2; }, f: function f() { try { if (!normalCompletion && it["return"] != null) it["return"](); } finally { if (didErr) throw err; } } }; }

function _unsupportedIterableToArray(o, minLen) { if (!o) return; if (typeof o === "string") return _arrayLikeToArray(o, minLen); var n = Object.prototype.toString.call(o).slice(8, -1); if (n === "Object" && o.constructor) n = o.constructor.name; if (n === "Map" || n === "Set") return Array.from(o); if (n === "Arguments" || /^(?:Ui|I)nt(?:8|16|32)(?:Clamped)?Array$/.test(n)) return _arrayLikeToArray(o, minLen); }

function _arrayLikeToArray(arr, len) { if (len == null || len > arr.length) len = arr.length; for (var i = 0, arr2 = new Array(len); i < len; i++) { arr2[i] = arr[i]; } return arr2; }

/**
 * @function extractBitrateMultiplier
 * @desc Extract bitrate multiplier from a string like "x0.2".
 * @return {Promise<Object, Error>} The float number after "x".
 * @private
 */
function extractBitrateMultiplier(input) {
  if (typeof input !== 'string' || !input.startsWith('x')) {
    _logger["default"].warning('Invalid bitrate multiplier input.');

    return 0;
  }

  return Number.parseFloat(input.replace(/^x/, ''));
} // eslint-disable-next-line require-jsdoc


function sortNumbers(x, y) {
  return x - y;
} // eslint-disable-next-line require-jsdoc


function sortResolutions(x, y) {
  if (x.width !== y.width) {
    return x.width - y.width;
  } else {
    return x.height - y.height;
  }
}
/**
 * @function convertToPublicationSettings
 * @desc Convert mediaInfo received from server to PublicationSettings.
 * @private
 */


function convertToPublicationSettings(mediaInfo) {
  var audio = [];
  var video = [];
  var audioCodec;
  var videoCodec;
  var resolution;
  var framerate;
  var bitrate;
  var keyFrameInterval;
  var rid;

  var _iterator = _createForOfIteratorHelper(mediaInfo.tracks),
      _step;

  try {
    for (_iterator.s(); !(_step = _iterator.n()).done;) {
      var track = _step.value;

      if (track.type === 'audio') {
        if (track.format) {
          audioCodec = new CodecModule.AudioCodecParameters(track.format.codec, track.format.channelNum, track.format.sampleRate);
        }

        var audioPublicationSettings = new PublicationModule.AudioPublicationSettings(audioCodec);
        audioPublicationSettings._trackId = track.id;
        audio.push(audioPublicationSettings);
      } else if (track.type === 'video') {
        if (track.format) {
          videoCodec = new CodecModule.VideoCodecParameters(track.format.codec, track.format.profile);
        }

        if (track.parameters) {
          if (track.parameters.resolution) {
            resolution = new MediaFormatModule.Resolution(track.parameters.resolution.width, track.parameters.resolution.height);
          }

          framerate = track.parameters.framerate;
          bitrate = track.parameters.bitrate * 1000;
          keyFrameInterval = track.parameters.keyFrameInterval;
        }

        if (track.rid) {
          rid = track.rid;
        }

        var videoPublicationSettings = new PublicationModule.VideoPublicationSettings(videoCodec, resolution, framerate, bitrate, keyFrameInterval, rid);
        videoPublicationSettings._trackId = track.id;
        video.push(videoPublicationSettings);
      }
    }
  } catch (err) {
    _iterator.e(err);
  } finally {
    _iterator.f();
  }

  return new PublicationModule.PublicationSettings(audio, video);
}
/**
 * @function convertToSubscriptionCapabilities
 * @desc Convert mediaInfo received from server to SubscriptionCapabilities.
 * @private
 */


function convertToSubscriptionCapabilities(mediaInfo) {
  var audio;
  var video;

  var _iterator2 = _createForOfIteratorHelper(mediaInfo.tracks),
      _step2;

  try {
    for (_iterator2.s(); !(_step2 = _iterator2.n()).done;) {
      var track = _step2.value;

      if (track.type === 'audio') {
        var audioCodecs = [];

        if (track.optional && track.optional.format) {
          var _iterator3 = _createForOfIteratorHelper(track.optional.format),
              _step3;

          try {
            for (_iterator3.s(); !(_step3 = _iterator3.n()).done;) {
              var audioCodecInfo = _step3.value;
              var audioCodec = new CodecModule.AudioCodecParameters(audioCodecInfo.codec, audioCodecInfo.channelNum, audioCodecInfo.sampleRate);
              audioCodecs.push(audioCodec);
            }
          } catch (err) {
            _iterator3.e(err);
          } finally {
            _iterator3.f();
          }
        }

        audioCodecs.sort();
        audio = new SubscriptionModule.AudioSubscriptionCapabilities(audioCodecs);
      } else if (track.type === 'video') {
        var videoCodecs = [];

        if (track.optional && track.optional.format) {
          var _iterator4 = _createForOfIteratorHelper(track.optional.format),
              _step4;

          try {
            for (_iterator4.s(); !(_step4 = _iterator4.n()).done;) {
              var videoCodecInfo = _step4.value;
              var videoCodec = new CodecModule.VideoCodecParameters(videoCodecInfo.codec, videoCodecInfo.profile);
              videoCodecs.push(videoCodec);
            }
          } catch (err) {
            _iterator4.e(err);
          } finally {
            _iterator4.f();
          }
        }

        videoCodecs.sort();
        var resolutions = Array.from(track.optional.parameters.resolution, function (r) {
          return new MediaFormatModule.Resolution(r.width, r.height);
        });
        resolutions.sort(sortResolutions);
        var bitrates = Array.from(track.optional.parameters.bitrate, function (bitrate) {
          return extractBitrateMultiplier(bitrate);
        });
        bitrates.push(1.0);
        bitrates.sort(sortNumbers);
        var frameRates = JSON.parse(JSON.stringify(track.optional.parameters.framerate));
        frameRates.sort(sortNumbers);
        var keyFrameIntervals = JSON.parse(JSON.stringify(track.optional.parameters.keyFrameInterval));
        keyFrameIntervals.sort(sortNumbers);
        video = new SubscriptionModule.VideoSubscriptionCapabilities(videoCodecs, resolutions, frameRates, bitrates, keyFrameIntervals);
      }
    }
  } catch (err) {
    _iterator2.e(err);
  } finally {
    _iterator2.f();
  }

  return new SubscriptionModule.SubscriptionCapabilities(audio, video);
}

},{"../base/codec.js":25,"../base/logger.js":28,"../base/mediaformat.js":29,"../base/publication.js":31,"./subscription.js":46,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/interopRequireWildcard":11}],46:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Subscription = exports.SubscriptionUpdateOptions = exports.VideoSubscriptionUpdateOptions = exports.SubscribeOptions = exports.VideoSubscriptionConstraints = exports.AudioSubscriptionConstraints = exports.SubscriptionCapabilities = exports.VideoSubscriptionCapabilities = exports.AudioSubscriptionCapabilities = void 0;

var _assertThisInitialized2 = _interopRequireDefault(require("@babel/runtime/helpers/assertThisInitialized"));

var _inherits2 = _interopRequireDefault(require("@babel/runtime/helpers/inherits"));

var _possibleConstructorReturn2 = _interopRequireDefault(require("@babel/runtime/helpers/possibleConstructorReturn"));

var _getPrototypeOf2 = _interopRequireDefault(require("@babel/runtime/helpers/getPrototypeOf"));

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var _event = require("../base/event.js");

function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = (0, _getPrototypeOf2["default"])(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return (0, _possibleConstructorReturn2["default"])(this, result); }; }

function _isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

/**
 * @class AudioSubscriptionCapabilities
 * @memberOf Owt.Conference
 * @classDesc Represents the audio capability for subscription.
 * @hideconstructor
 */
var AudioSubscriptionCapabilities = // eslint-disable-next-line require-jsdoc
function AudioSubscriptionCapabilities(codecs) {
  (0, _classCallCheck2["default"])(this, AudioSubscriptionCapabilities);

  /**
   * @member {Array.<Owt.Base.AudioCodecParameters>} codecs
   * @instance
   * @memberof Owt.Conference.AudioSubscriptionCapabilities
   */
  this.codecs = codecs;
};
/**
 * @class VideoSubscriptionCapabilities
 * @memberOf Owt.Conference
 * @classDesc Represents the video capability for subscription.
 * @hideconstructor
 */


exports.AudioSubscriptionCapabilities = AudioSubscriptionCapabilities;

var VideoSubscriptionCapabilities = // eslint-disable-next-line require-jsdoc
function VideoSubscriptionCapabilities(codecs, resolutions, frameRates, bitrateMultipliers, keyFrameIntervals) {
  (0, _classCallCheck2["default"])(this, VideoSubscriptionCapabilities);

  /**
   * @member {Array.<Owt.Base.VideoCodecParameters>} codecs
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionCapabilities
   */
  this.codecs = codecs;
  /**
   * @member {Array.<Owt.Base.Resolution>} resolutions
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionCapabilities
   */

  this.resolutions = resolutions;
  /**
   * @member {Array.<number>} frameRates
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionCapabilities
   */

  this.frameRates = frameRates;
  /**
   * @member {Array.<number>} bitrateMultipliers
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionCapabilities
   */

  this.bitrateMultipliers = bitrateMultipliers;
  /**
   * @member {Array.<number>} keyFrameIntervals
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionCapabilities
   */

  this.keyFrameIntervals = keyFrameIntervals;
};
/**
 * @class SubscriptionCapabilities
 * @memberOf Owt.Conference
 * @classDesc Represents the capability for subscription.
 * @hideconstructor
 */


exports.VideoSubscriptionCapabilities = VideoSubscriptionCapabilities;

var SubscriptionCapabilities = // eslint-disable-next-line require-jsdoc
function SubscriptionCapabilities(audio, video) {
  (0, _classCallCheck2["default"])(this, SubscriptionCapabilities);

  /**
   * @member {?Owt.Conference.AudioSubscriptionCapabilities} audio
   * @instance
   * @memberof Owt.Conference.SubscriptionCapabilities
   */
  this.audio = audio;
  /**
   * @member {?Owt.Conference.VideoSubscriptionCapabilities} video
   * @instance
   * @memberof Owt.Conference.SubscriptionCapabilities
   */

  this.video = video;
};
/**
 * @class AudioSubscriptionConstraints
 * @memberOf Owt.Conference
 * @classDesc Represents the audio constraints for subscription.
 * @hideconstructor
 */


exports.SubscriptionCapabilities = SubscriptionCapabilities;

var AudioSubscriptionConstraints = // eslint-disable-next-line require-jsdoc
function AudioSubscriptionConstraints(codecs) {
  (0, _classCallCheck2["default"])(this, AudioSubscriptionConstraints);

  /**
   * @member {?Array.<Owt.Base.AudioCodecParameters>} codecs
   * @instance
   * @memberof Owt.Conference.AudioSubscriptionConstraints
   * @desc Codecs accepted. If none of `codecs` supported by both sides, connection fails. Leave it undefined will use all possible codecs.
   */
  this.codecs = codecs;
};
/**
 * @class VideoSubscriptionConstraints
 * @memberOf Owt.Conference
 * @classDesc Represents the video constraints for subscription.
 * @hideconstructor
 */


exports.AudioSubscriptionConstraints = AudioSubscriptionConstraints;

var VideoSubscriptionConstraints = // eslint-disable-next-line require-jsdoc
function VideoSubscriptionConstraints(codecs, resolution, frameRate, bitrateMultiplier, keyFrameInterval, rid) {
  (0, _classCallCheck2["default"])(this, VideoSubscriptionConstraints);

  /**
   * @member {?Array.<Owt.Base.VideoCodecParameters>} codecs
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionConstraints
   * @desc Codecs accepted. If none of `codecs` supported by both sides, connection fails. Leave it undefined will use all possible codecs.
   */
  this.codecs = codecs;
  /**
   * @member {?Owt.Base.Resolution} resolution
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionConstraints
   * @desc Only resolutions listed in Owt.Conference.VideoSubscriptionCapabilities are allowed.
   */

  this.resolution = resolution;
  /**
   * @member {?number} frameRate
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionConstraints
   * @desc Only frameRates listed in Owt.Conference.VideoSubscriptionCapabilities are allowed.
   */

  this.frameRate = frameRate;
  /**
   * @member {?number} bitrateMultiplier
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionConstraints
   * @desc Only bitrateMultipliers listed in Owt.Conference.VideoSubscriptionCapabilities are allowed.
   */

  this.bitrateMultiplier = bitrateMultiplier;
  /**
   * @member {?number} keyFrameInterval
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionConstraints
   * @desc Only keyFrameIntervals listed in Owt.Conference.VideoSubscriptionCapabilities are allowed.
   */

  this.keyFrameInterval = keyFrameInterval;
  /**
   * @member {?number} rid
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionConstraints
   * @desc Restriction identifier to identify the RTP Streams within an RTP session. When rid is specified, other constraints will be ignored.
   */

  this.rid = rid;
};
/**
 * @class SubscribeOptions
 * @memberOf Owt.Conference
 * @classDesc SubscribeOptions defines options for subscribing a Owt.Base.RemoteStream.
 */


exports.VideoSubscriptionConstraints = VideoSubscriptionConstraints;

var SubscribeOptions = // eslint-disable-next-line require-jsdoc
function SubscribeOptions(audio, video, transport) {
  (0, _classCallCheck2["default"])(this, SubscribeOptions);

  /**
   * @member {?Owt.Conference.AudioSubscriptionConstraints} audio
   * @instance
   * @memberof Owt.Conference.SubscribeOptions
   */
  this.audio = audio;
  /**
   * @member {?Owt.Conference.VideoSubscriptionConstraints} video
   * @instance
   * @memberof Owt.Conference.SubscribeOptions
   */

  this.video = video;
  /**
   * @member {?Owt.Base.TransportConstraints} transport
   * @instance
   * @memberof Owt.Conference.SubscribeOptions
   */

  this.transport = transport;
};
/**
 * @class VideoSubscriptionUpdateOptions
 * @memberOf Owt.Conference
 * @classDesc VideoSubscriptionUpdateOptions defines options for updating a subscription's video part.
 * @hideconstructor
 */


exports.SubscribeOptions = SubscribeOptions;

var VideoSubscriptionUpdateOptions = // eslint-disable-next-line require-jsdoc
function VideoSubscriptionUpdateOptions() {
  (0, _classCallCheck2["default"])(this, VideoSubscriptionUpdateOptions);

  /**
   * @member {?Owt.Base.Resolution} resolution
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionUpdateOptions
   * @desc Only resolutions listed in VideoSubscriptionCapabilities are allowed.
   */
  this.resolution = undefined;
  /**
   * @member {?number} frameRates
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionUpdateOptions
   * @desc Only frameRates listed in VideoSubscriptionCapabilities are allowed.
   */

  this.frameRate = undefined;
  /**
   * @member {?number} bitrateMultipliers
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionUpdateOptions
   * @desc Only bitrateMultipliers listed in VideoSubscriptionCapabilities are allowed.
   */

  this.bitrateMultipliers = undefined;
  /**
   * @member {?number} keyFrameIntervals
   * @instance
   * @memberof Owt.Conference.VideoSubscriptionUpdateOptions
   * @desc Only keyFrameIntervals listed in VideoSubscriptionCapabilities are allowed.
   */

  this.keyFrameInterval = undefined;
};
/**
 * @class SubscriptionUpdateOptions
 * @memberOf Owt.Conference
 * @classDesc SubscriptionUpdateOptions defines options for updating a subscription.
 * @hideconstructor
 */


exports.VideoSubscriptionUpdateOptions = VideoSubscriptionUpdateOptions;

var SubscriptionUpdateOptions = // eslint-disable-next-line require-jsdoc
function SubscriptionUpdateOptions() {
  (0, _classCallCheck2["default"])(this, SubscriptionUpdateOptions);

  /**
   * @member {?VideoSubscriptionUpdateOptions} video
   * @instance
   * @memberof Owt.Conference.SubscriptionUpdateOptions
   */
  this.video = undefined;
};
/**
 * @class Subscription
 * @memberof Owt.Conference
 * @classDesc Subscription is a receiver for receiving a stream.
 * Events:
 *
 * | Event Name      | Argument Type    | Fired when       |
 * | ----------------| ---------------- | ---------------- |
 * | ended           | Event            | Subscription is ended. |
 * | error           | ErrorEvent       | An error occurred on the subscription. |
 * | mute            | MuteEvent        | Publication is muted. Remote side stopped sending audio and/or video data. |
 * | unmute          | MuteEvent        | Publication is unmuted. Remote side continued sending audio and/or video data. |
 *
 * @extends Owt.Base.EventDispatcher
 * @hideconstructor
 */


exports.SubscriptionUpdateOptions = SubscriptionUpdateOptions;

var Subscription = /*#__PURE__*/function (_EventDispatcher) {
  (0, _inherits2["default"])(Subscription, _EventDispatcher);

  var _super = _createSuper(Subscription);

  // eslint-disable-next-line require-jsdoc
  function Subscription(id, stream, transport, stop, getStats, mute, unmute, applyOptions) {
    var _this;

    (0, _classCallCheck2["default"])(this, Subscription);
    _this = _super.call(this);

    if (!id) {
      throw new TypeError('ID cannot be null or undefined.');
    }
    /**
     * @member {string} id
     * @instance
     * @memberof Owt.Conference.Subscription
     */


    Object.defineProperty((0, _assertThisInitialized2["default"])(_this), 'id', {
      configurable: false,
      writable: false,
      value: id
    });
    /**
     * @member {MediaStream | BidirectionalStream} stream
     * @instance
     * @memberof Owt.Conference.Subscription
     */

    Object.defineProperty((0, _assertThisInitialized2["default"])(_this), 'stream', {
      configurable: false,
      // TODO: It should be a readonly property, but current implementation
      // creates Subscription after receiving 'ready' from server. At this time,
      // MediaStream may not be available.
      writable: true,
      value: stream
    });
    /**
     * @member {Owt.Base.TransportSettings} transport
     * @instance
     * @readonly
     * @desc Transport settings for the subscription.
     * @memberof Owt.Base.Subscription
     */

    Object.defineProperty((0, _assertThisInitialized2["default"])(_this), 'transport', {
      configurable: false,
      writable: false,
      value: transport
    });
    /**
     * @function stop
     * @instance
     * @desc Stop certain subscription. Once a subscription is stopped, it cannot be recovered.
     * @memberof Owt.Conference.Subscription
     * @returns {undefined}
     */

    _this.stop = stop;
    /**
     * @function getStats
     * @instance
     * @desc Get stats of underlying PeerConnection.
     * @memberof Owt.Conference.Subscription
     * @returns {Promise<RTCStatsReport, Error>}
     */

    _this.getStats = getStats;
    /**
     * @function mute
     * @instance
     * @desc Stop reeving data from remote endpoint.
     * @memberof Owt.Conference.Subscription
     * @param {Owt.Base.TrackKind } kind Kind of tracks to be muted.
     * @returns {Promise<undefined, Error>}
     */

    _this.mute = mute;
    /**
     * @function unmute
     * @instance
     * @desc Continue reeving data from remote endpoint.
     * @memberof Owt.Conference.Subscription
     * @param {Owt.Base.TrackKind } kind Kind of tracks to be unmuted.
     * @returns {Promise<undefined, Error>}
     */

    _this.unmute = unmute;
    /**
     * @function applyOptions
     * @instance
     * @desc Update subscription with given options.
     * @memberof Owt.Conference.Subscription
     * @param {Owt.Conference.SubscriptionUpdateOptions } options Subscription update options.
     * @returns {Promise<undefined, Error>}
     */

    _this.applyOptions = applyOptions; // Track is not defined in server protocol. So these IDs are equal to their
    // stream's ID at this time.

    _this._audioTrackId = undefined;
    _this._videoTrackId = undefined;
    return _this;
  }

  return Subscription;
}(_event.EventDispatcher);

exports.Subscription = Subscription;

},{"../base/event.js":26,"@babel/runtime/helpers/assertThisInitialized":3,"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/getPrototypeOf":8,"@babel/runtime/helpers/inherits":9,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/possibleConstructorReturn":16}],47:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';

var _interopRequireWildcard = require("@babel/runtime/helpers/interopRequireWildcard");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Conference = exports.P2P = exports.Base = void 0;

var base = _interopRequireWildcard(require("./base/export.js"));

var p2p = _interopRequireWildcard(require("./p2p/export.js"));

var conference = _interopRequireWildcard(require("./conference/export.js"));

/**
 * Base objects for both P2P and conference.
 * @namespace Owt.Base
 */
var Base = base;
/**
 * P2P WebRTC connections.
 * @namespace Owt.P2P
 */

exports.Base = Base;
var P2P = p2p;
/**
 * WebRTC connections with conference server.
 * @namespace Owt.Conference
 */

exports.P2P = P2P;
var Conference = conference;
exports.Conference = Conference;

},{"./base/export.js":27,"./conference/export.js":39,"./p2p/export.js":49,"@babel/runtime/helpers/interopRequireWildcard":11}],48:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getErrorByCode = getErrorByCode;
exports.P2PError = exports.errors = void 0;

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var _inherits2 = _interopRequireDefault(require("@babel/runtime/helpers/inherits"));

var _possibleConstructorReturn2 = _interopRequireDefault(require("@babel/runtime/helpers/possibleConstructorReturn"));

var _getPrototypeOf2 = _interopRequireDefault(require("@babel/runtime/helpers/getPrototypeOf"));

var _wrapNativeSuper2 = _interopRequireDefault(require("@babel/runtime/helpers/wrapNativeSuper"));

function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = (0, _getPrototypeOf2["default"])(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return (0, _possibleConstructorReturn2["default"])(this, result); }; }

function _isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

var errors = {
  // 2100-2999 for P2P errors
  // 2100-2199 for connection errors
  // 2100-2109 for server errors
  P2P_CONN_SERVER_UNKNOWN: {
    code: 2100,
    message: 'Server unknown error.'
  },
  P2P_CONN_SERVER_UNAVAILABLE: {
    code: 2101,
    message: 'Server is unavaliable.'
  },
  P2P_CONN_SERVER_BUSY: {
    code: 2102,
    message: 'Server is too busy.'
  },
  P2P_CONN_SERVER_NOT_SUPPORTED: {
    code: 2103,
    message: 'Method has not been supported by server.'
  },
  // 2110-2119 for client errors
  P2P_CONN_CLIENT_UNKNOWN: {
    code: 2110,
    message: 'Client unknown error.'
  },
  P2P_CONN_CLIENT_NOT_INITIALIZED: {
    code: 2111,
    message: 'Connection is not initialized.'
  },
  // 2120-2129 for authentication errors
  P2P_CONN_AUTH_UNKNOWN: {
    code: 2120,
    message: 'Authentication unknown error.'
  },
  P2P_CONN_AUTH_FAILED: {
    code: 2121,
    message: 'Wrong username or token.'
  },
  // 2200-2299 for message transport errors
  P2P_MESSAGING_TARGET_UNREACHABLE: {
    code: 2201,
    message: 'Remote user cannot be reached.'
  },
  P2P_CLIENT_DENIED: {
    code: 2202,
    message: 'User is denied.'
  },
  // 2301-2399 for chat room errors
  // 2401-2499 for client errors
  P2P_CLIENT_UNKNOWN: {
    code: 2400,
    message: 'Unknown errors.'
  },
  P2P_CLIENT_UNSUPPORTED_METHOD: {
    code: 2401,
    message: 'This method is unsupported in current browser.'
  },
  P2P_CLIENT_ILLEGAL_ARGUMENT: {
    code: 2402,
    message: 'Illegal argument.'
  },
  P2P_CLIENT_INVALID_STATE: {
    code: 2403,
    message: 'Invalid peer state.'
  },
  P2P_CLIENT_NOT_ALLOWED: {
    code: 2404,
    message: 'Remote user is not allowed.'
  },
  // 2501-2599 for WebRTC erros.
  P2P_WEBRTC_UNKNOWN: {
    code: 2500,
    message: 'WebRTC error.'
  },
  P2P_WEBRTC_SDP: {
    code: 2502,
    message: 'SDP error.'
  }
};
/**
 * @function getErrorByCode
 * @desc Get error object by error code.
 * @param {string} errorCode Error code.
 * @return {Owt.P2P.Error} Error object
 * @private
 */

exports.errors = errors;

function getErrorByCode(errorCode) {
  var codeErrorMap = {
    2100: errors.P2P_CONN_SERVER_UNKNOWN,
    2101: errors.P2P_CONN_SERVER_UNAVAILABLE,
    2102: errors.P2P_CONN_SERVER_BUSY,
    2103: errors.P2P_CONN_SERVER_NOT_SUPPORTED,
    2110: errors.P2P_CONN_CLIENT_UNKNOWN,
    2111: errors.P2P_CONN_CLIENT_NOT_INITIALIZED,
    2120: errors.P2P_CONN_AUTH_UNKNOWN,
    2121: errors.P2P_CONN_AUTH_FAILED,
    2201: errors.P2P_MESSAGING_TARGET_UNREACHABLE,
    2400: errors.P2P_CLIENT_UNKNOWN,
    2401: errors.P2P_CLIENT_UNSUPPORTED_METHOD,
    2402: errors.P2P_CLIENT_ILLEGAL_ARGUMENT,
    2403: errors.P2P_CLIENT_INVALID_STATE,
    2404: errors.P2P_CLIENT_NOT_ALLOWED,
    2500: errors.P2P_WEBRTC_UNKNOWN,
    2501: errors.P2P_WEBRTC_SDP
  };
  return codeErrorMap[errorCode];
}
/**
 * @class P2PError
 * @classDesc The P2PError object represents an error in P2P mode.
 * @memberOf Owt.P2P
 * @hideconstructor
 */


var P2PError = /*#__PURE__*/function (_Error) {
  (0, _inherits2["default"])(P2PError, _Error);

  var _super = _createSuper(P2PError);

  // eslint-disable-next-line require-jsdoc
  function P2PError(error, message) {
    var _this;

    (0, _classCallCheck2["default"])(this, P2PError);
    _this = _super.call(this, message);

    if (typeof error === 'number') {
      _this.code = error;
    } else {
      _this.code = error.code;
    }

    return _this;
  }

  return P2PError;
}( /*#__PURE__*/(0, _wrapNativeSuper2["default"])(Error));

exports.P2PError = P2PError;

},{"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/getPrototypeOf":8,"@babel/runtime/helpers/inherits":9,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/possibleConstructorReturn":16,"@babel/runtime/helpers/wrapNativeSuper":21}],49:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
'use strict';

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
Object.defineProperty(exports, "P2PClient", {
  enumerable: true,
  get: function get() {
    return _p2pclient["default"];
  }
});
Object.defineProperty(exports, "P2PError", {
  enumerable: true,
  get: function get() {
    return _error.P2PError;
  }
});

var _p2pclient = _interopRequireDefault(require("./p2pclient.js"));

var _error = require("./error.js");

},{"./error.js":48,"./p2pclient.js":50,"@babel/runtime/helpers/interopRequireDefault":10}],50:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/* global Map, Promise */
'use strict';

var _interopRequireWildcard = require("@babel/runtime/helpers/interopRequireWildcard");

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports["default"] = void 0;

var _logger = _interopRequireDefault(require("../base/logger.js"));

var _event = require("../base/event.js");

var ErrorModule = _interopRequireWildcard(require("./error.js"));

var _peerconnectionChannel = _interopRequireDefault(require("./peerconnection-channel.js"));

var ConnectionState = {
  READY: 1,
  CONNECTING: 2,
  CONNECTED: 3
};
/* eslint-disable no-unused-vars */

/**
 * @class P2PClientConfiguration
 * @classDesc Configuration for P2PClient.
 * @memberOf Owt.P2P
 * @hideconstructor
 */

var P2PClientConfiguration = function P2PClientConfiguration() {
  /**
   * @member {?Array<Owt.Base.AudioEncodingParameters>} audioEncoding
   * @instance
   * @desc Encoding parameters for publishing audio tracks.
   * @memberof Owt.P2P.P2PClientConfiguration
   */
  this.audioEncoding = undefined;
  /**
   * @member {?Array<Owt.Base.VideoEncodingParameters>} videoEncoding
   * @instance
   * @desc Encoding parameters for publishing video tracks.
   * @memberof Owt.P2P.P2PClientConfiguration
   */

  this.videoEncoding = undefined;
  /**
   * @member {?RTCConfiguration} rtcConfiguration
   * @instance
   * @memberof Owt.P2P.P2PClientConfiguration
   * @desc It will be used for creating PeerConnection.
   * @see {@link https://www.w3.org/TR/webrtc/#rtcconfiguration-dictionary|RTCConfiguration Dictionary of WebRTC 1.0}.
   * @example
   * // Following object can be set to p2pClientConfiguration.rtcConfiguration.
   * {
   *   iceServers: [{
   *      urls: "stun:example.com:3478"
   *   }, {
   *     urls: [
   *       "turn:example.com:3478?transport=udp",
   *       "turn:example.com:3478?transport=tcp"
   *     ],
   *      credential: "password",
   *      username: "username"
   *   }
   * }
   */

  this.rtcConfiguration = undefined;
};
/* eslint-enable no-unused-vars */

/**
 * @class P2PClient
 * @classDesc The P2PClient handles PeerConnections between different clients.
 * Events:
 *
 * | Event Name            | Argument Type    | Fired when       |
 * | --------------------- | ---------------- | ---------------- |
 * | streamadded           | StreamEvent      | A new stream is sent from remote endpoint. |
 * | messagereceived       | MessageEvent     | A new message is received. |
 * | serverdisconnected    | OwtEvent         | Disconnected from signaling server. |
 *
 * @memberof Owt.P2P
 * @extends Owt.Base.EventDispatcher
 * @constructor
 * @param {?Owt.P2P.P2PClientConfiguration } configuration Configuration for Owt.P2P.P2PClient.
 * @param {Object} signalingChannel A channel for sending and receiving signaling messages.
 */


var P2PClient = function P2PClient(configuration, signalingChannel) {
  Object.setPrototypeOf(this, new _event.EventDispatcher());
  var config = configuration;
  var signaling = signalingChannel;
  var channels = new Map(); // Map of PeerConnectionChannels.

  var connectionIds = new Map(); // Key is remote user ID, value is current session ID.

  var self = this;
  var state = ConnectionState.READY;
  var myId;

  signaling.onMessage = function (origin, message) {
    _logger["default"].debug('Received signaling message from ' + origin + ': ' + message);

    var data = JSON.parse(message);
    var connectionId = data.connectionId;

    if (self.allowedRemoteIds.indexOf(origin) < 0) {
      sendSignalingMessage(origin, data.connectionId, 'chat-closed', ErrorModule.errors.P2P_CLIENT_DENIED);
      return;
    }

    if (connectionIds.has(origin) && connectionIds.get(origin) !== connectionId && !isPolitePeer(origin)) {
      _logger["default"].warning( // eslint-disable-next-line max-len
      'Collision detected, ignore this message because current endpoint is impolite peer.');

      return;
    }

    if (data.type === 'chat-closed') {
      if (channels.has(origin)) {
        getOrCreateChannel(origin, connectionId).onMessage(data);
        channels["delete"](origin);
      }

      return;
    }

    getOrCreateChannel(origin, connectionId).onMessage(data);
  };

  signaling.onServerDisconnected = function () {
    state = ConnectionState.READY;
    self.dispatchEvent(new _event.OwtEvent('serverdisconnected'));
  };
  /**
   * @member {array} allowedRemoteIds
   * @memberof Owt.P2P.P2PClient
   * @instance
   * @desc Only allowed remote endpoint IDs are able to publish stream or send message to current endpoint. Removing an ID from allowedRemoteIds does stop existing connection with certain endpoint. Please call stop to stop the PeerConnection.
   */


  this.allowedRemoteIds = [];
  /**
   * @function connect
   * @instance
   * @desc Connect to signaling server. Since signaling can be customized, this method does not define how a token looks like. SDK passes token to signaling channel without changes.
   * @memberof Owt.P2P.P2PClient
   * @param {string} token A token for connecting to signaling server. The format of this token depends on signaling server's requirement.
   * @return {Promise<object, Error>} It returns a promise resolved with an object returned by signaling channel once signaling channel reports connection has been established.
   */

  this.connect = function (token) {
    if (state === ConnectionState.READY) {
      state = ConnectionState.CONNECTING;
    } else {
      _logger["default"].warning('Invalid connection state: ' + state);

      return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_INVALID_STATE));
    }

    return new Promise(function (resolve, reject) {
      signaling.connect(token).then(function (id) {
        myId = id;
        state = ConnectionState.CONNECTED;
        resolve(myId);
      }, function (errCode) {
        reject(new ErrorModule.P2PError(ErrorModule.getErrorByCode(errCode)));
      });
    });
  };
  /**
   * @function disconnect
   * @instance
   * @desc Disconnect from the signaling channel. It stops all existing sessions with remote endpoints.
   * @memberof Owt.P2P.P2PClient
   */


  this.disconnect = function () {
    if (state == ConnectionState.READY) {
      return;
    }

    channels.forEach(function (channel) {
      channel.stop();
    });
    channels.clear();
    signaling.disconnect();
  };
  /**
   * @function publish
   * @instance
   * @desc Publish a stream to a remote endpoint.
   * @memberof Owt.P2P.P2PClient
   * @param {string} remoteId Remote endpoint's ID.
   * @param {Owt.Base.LocalStream} stream An Owt.Base.LocalStream to be published.
   * @return {Promise<Owt.Base.Publication, Error>} A promised that resolves when remote side received the certain stream. However, remote endpoint may not display this stream, or ignore it.
   */


  this.publish = function (remoteId, stream) {
    if (state !== ConnectionState.CONNECTED) {
      return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_INVALID_STATE, 'P2P Client is not connected to signaling channel.'));
    }

    if (this.allowedRemoteIds.indexOf(remoteId) < 0) {
      return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_NOT_ALLOWED));
    }

    return Promise.resolve(getOrCreateChannel(remoteId).publish(stream));
  };
  /**
   * @function send
   * @instance
   * @desc Send a message to remote endpoint.
   * @memberof Owt.P2P.P2PClient
   * @param {string} remoteId Remote endpoint's ID.
   * @param {string} message Message to be sent. It should be a string.
   * @return {Promise<undefined, Error>} It returns a promise resolved when remote endpoint received certain message.
   */


  this.send = function (remoteId, message) {
    if (state !== ConnectionState.CONNECTED) {
      return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_INVALID_STATE, 'P2P Client is not connected to signaling channel.'));
    }

    if (this.allowedRemoteIds.indexOf(remoteId) < 0) {
      return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_NOT_ALLOWED));
    }

    return Promise.resolve(getOrCreateChannel(remoteId).send(message));
  };
  /**
   * @function stop
   * @instance
   * @desc Clean all resources associated with given remote endpoint. It may include RTCPeerConnection, RTCRtpTransceiver and RTCDataChannel. It still possible to publish a stream, or send a message to given remote endpoint after stop.
   * @memberof Owt.P2P.P2PClient
   * @param {string} remoteId Remote endpoint's ID.
   * @return {undefined}
   */


  this.stop = function (remoteId) {
    if (!channels.has(remoteId)) {
      _logger["default"].warning('No PeerConnection between current endpoint and specific remote ' + 'endpoint.');

      return;
    }

    channels.get(remoteId).stop();
    channels["delete"](remoteId);
  };
  /**
   * @function getStats
   * @instance
   * @desc Get stats of underlying PeerConnection.
   * @memberof Owt.P2P.P2PClient
   * @param {string} remoteId Remote endpoint's ID.
   * @return {Promise<RTCStatsReport, Error>} It returns a promise resolved with an RTCStatsReport or reject with an Error if there is no connection with specific user.
   */


  this.getStats = function (remoteId) {
    if (!channels.has(remoteId)) {
      return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_INVALID_STATE, 'No PeerConnection between current endpoint and specific remote ' + 'endpoint.'));
    }

    return channels.get(remoteId).getStats();
  };
  /**
   * @function getPeerConnection
   * @instance
   * @desc Get underlying PeerConnection.
   * @memberof Owt.P2P.P2PClient
   * @param {string} remoteId Remote endpoint's ID.
   * @return {Promise<RTCPeerConnection, Error>} It returns a promise resolved
   *     with an RTCPeerConnection or reject with an Error if there is no
   *     connection with specific user.
   * @private
   */


  this.getPeerConnection = function (remoteId) {
    if (!channels.has(remoteId)) {
      return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_INVALID_STATE, 'No PeerConnection between current endpoint and specific remote ' + 'endpoint.'));
    }

    return channels.get(remoteId).peerConnection;
  };

  var sendSignalingMessage = function sendSignalingMessage(remoteId, connectionId, type, message) {
    var msg = {
      type: type,
      connectionId: connectionId
    };

    if (message) {
      msg.data = message;
    }

    return signaling.send(remoteId, JSON.stringify(msg))["catch"](function (e) {
      if (typeof e === 'number') {
        throw ErrorModule.getErrorByCode(e);
      }
    });
  }; // Return true if current endpoint is an impolite peer, which controls the
  // session.


  var isPolitePeer = function isPolitePeer(remoteId) {
    return myId < remoteId;
  }; // If a connection with remoteId with a different session ID exists, it will
  // be stopped and a new connection will be created.


  var getOrCreateChannel = function getOrCreateChannel(remoteId, connectionId) {
    // If `connectionId` is not defined, use the latest one or generate a new
    // one.
    if (!connectionId && connectionIds.has(remoteId)) {
      connectionId = connectionIds.get(remoteId);
    } // Delete old channel if connection doesn't match.


    if (connectionIds.has(remoteId) && connectionIds.get(remoteId) != connectionId) {
      self.stop(remoteId);
    }

    if (!connectionId) {
      var connectionIdLimit = 100000;
      connectionId = Math.round(Math.random() * connectionIdLimit);
    }

    connectionIds.set(remoteId, connectionId);

    if (!channels.has(remoteId)) {
      // Construct an signaling sender/receiver for P2PPeerConnection.
      var signalingForChannel = Object.create(_event.EventDispatcher);
      signalingForChannel.sendSignalingMessage = sendSignalingMessage;
      var pcc = new _peerconnectionChannel["default"](config, myId, remoteId, connectionId, signalingForChannel);
      pcc.addEventListener('streamadded', function (streamEvent) {
        self.dispatchEvent(streamEvent);
      });
      pcc.addEventListener('messagereceived', function (messageEvent) {
        self.dispatchEvent(messageEvent);
      });
      pcc.addEventListener('ended', function () {
        if (channels.has(remoteId)) {
          channels["delete"](remoteId);
        }

        connectionIds["delete"](remoteId);
      });
      channels.set(remoteId, pcc);
    }

    return channels.get(remoteId);
  };
};

var _default = P2PClient;
exports["default"] = _default;

},{"../base/event.js":26,"../base/logger.js":28,"./error.js":48,"./peerconnection-channel.js":51,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/interopRequireWildcard":11}],51:[function(require,module,exports){
// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// This file doesn't have public APIs.

/* eslint-disable valid-jsdoc */

/* eslint-disable require-jsdoc */

/* global Event, Map, Promise, RTCIceCandidate, navigator */
'use strict';

var _interopRequireWildcard = require("@babel/runtime/helpers/interopRequireWildcard");

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports["default"] = exports.P2PPeerConnectionChannelEvent = void 0;

var _slicedToArray2 = _interopRequireDefault(require("@babel/runtime/helpers/slicedToArray"));

var _typeof2 = _interopRequireDefault(require("@babel/runtime/helpers/typeof"));

var _createClass2 = _interopRequireDefault(require("@babel/runtime/helpers/createClass"));

var _classCallCheck2 = _interopRequireDefault(require("@babel/runtime/helpers/classCallCheck"));

var _inherits2 = _interopRequireDefault(require("@babel/runtime/helpers/inherits"));

var _possibleConstructorReturn2 = _interopRequireDefault(require("@babel/runtime/helpers/possibleConstructorReturn"));

var _getPrototypeOf2 = _interopRequireDefault(require("@babel/runtime/helpers/getPrototypeOf"));

var _wrapNativeSuper2 = _interopRequireDefault(require("@babel/runtime/helpers/wrapNativeSuper"));

var _logger = _interopRequireDefault(require("../base/logger.js"));

var _event = require("../base/event.js");

var _publication = require("../base/publication.js");

var Utils = _interopRequireWildcard(require("../base/utils.js"));

var ErrorModule = _interopRequireWildcard(require("./error.js"));

var StreamModule = _interopRequireWildcard(require("../base/stream.js"));

var SdpUtils = _interopRequireWildcard(require("../base/sdputils.js"));

var _transport = require("../base/transport.js");

function _createForOfIteratorHelper(o, allowArrayLike) { var it; if (typeof Symbol === "undefined" || o[Symbol.iterator] == null) { if (Array.isArray(o) || (it = _unsupportedIterableToArray(o)) || allowArrayLike && o && typeof o.length === "number") { if (it) o = it; var i = 0; var F = function F() {}; return { s: F, n: function n() { if (i >= o.length) return { done: true }; return { done: false, value: o[i++] }; }, e: function e(_e) { throw _e; }, f: F }; } throw new TypeError("Invalid attempt to iterate non-iterable instance.\nIn order to be iterable, non-array objects must have a [Symbol.iterator]() method."); } var normalCompletion = true, didErr = false, err; return { s: function s() { it = o[Symbol.iterator](); }, n: function n() { var step = it.next(); normalCompletion = step.done; return step; }, e: function e(_e2) { didErr = true; err = _e2; }, f: function f() { try { if (!normalCompletion && it["return"] != null) it["return"](); } finally { if (didErr) throw err; } } }; }

function _unsupportedIterableToArray(o, minLen) { if (!o) return; if (typeof o === "string") return _arrayLikeToArray(o, minLen); var n = Object.prototype.toString.call(o).slice(8, -1); if (n === "Object" && o.constructor) n = o.constructor.name; if (n === "Map" || n === "Set") return Array.from(o); if (n === "Arguments" || /^(?:Ui|I)nt(?:8|16|32)(?:Clamped)?Array$/.test(n)) return _arrayLikeToArray(o, minLen); }

function _arrayLikeToArray(arr, len) { if (len == null || len > arr.length) len = arr.length; for (var i = 0, arr2 = new Array(len); i < len; i++) { arr2[i] = arr[i]; } return arr2; }

function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = (0, _getPrototypeOf2["default"])(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return (0, _possibleConstructorReturn2["default"])(this, result); }; }

function _isNativeReflectConstruct() { if (typeof Reflect === "undefined" || !Reflect.construct) return false; if (Reflect.construct.sham) return false; if (typeof Proxy === "function") return true; try { Date.prototype.toString.call(Reflect.construct(Date, [], function () {})); return true; } catch (e) { return false; } }

/**
 * @class P2PPeerConnectionChannelEvent
 * @desc Event for Stream.
 * @memberOf Owt.P2P
 * @private
 * */
var P2PPeerConnectionChannelEvent = /*#__PURE__*/function (_Event) {
  (0, _inherits2["default"])(P2PPeerConnectionChannelEvent, _Event);

  var _super = _createSuper(P2PPeerConnectionChannelEvent);

  /* eslint-disable-next-line require-jsdoc */
  function P2PPeerConnectionChannelEvent(init) {
    var _this;

    (0, _classCallCheck2["default"])(this, P2PPeerConnectionChannelEvent);
    _this = _super.call(this, init);
    _this.stream = init.stream;
    return _this;
  }

  return P2PPeerConnectionChannelEvent;
}( /*#__PURE__*/(0, _wrapNativeSuper2["default"])(Event));

exports.P2PPeerConnectionChannelEvent = P2PPeerConnectionChannelEvent;
var DataChannelLabel = {
  MESSAGE: 'message',
  FILE: 'file'
};
var SignalingType = {
  DENIED: 'chat-denied',
  CLOSED: 'chat-closed',
  NEGOTIATION_NEEDED: 'chat-negotiation-needed',
  TRACK_SOURCES: 'chat-track-sources',
  STREAM_INFO: 'chat-stream-info',
  SDP: 'chat-signal',
  TRACKS_ADDED: 'chat-tracks-added',
  TRACKS_REMOVED: 'chat-tracks-removed',
  DATA_RECEIVED: 'chat-data-received',
  UA: 'chat-ua'
};
var sysInfo = Utils.sysInfo();
/**
 * @class P2PPeerConnectionChannel
 * @desc A P2PPeerConnectionChannel manages a PeerConnection object, handles all
 * interactions between this endpoint (local) and a remote endpoint. Only one
 * PeerConnectionChannel is alive for a local - remote endpoint pair at any
 * given time.
 * @memberOf Owt.P2P
 * @private
 */

var P2PPeerConnectionChannel = /*#__PURE__*/function (_EventDispatcher) {
  (0, _inherits2["default"])(P2PPeerConnectionChannel, _EventDispatcher);

  var _super2 = _createSuper(P2PPeerConnectionChannel);

  // |signaling| is an object has a method |sendSignalingMessage|.

  /* eslint-disable-next-line require-jsdoc */
  function P2PPeerConnectionChannel(config, localId, remoteId, connectionId, signaling) {
    var _this2;

    (0, _classCallCheck2["default"])(this, P2PPeerConnectionChannel);
    _this2 = _super2.call(this);
    _this2._config = config;
    _this2._remoteId = remoteId;
    _this2._connectionId = connectionId;
    _this2._signaling = signaling;
    _this2._pc = null;
    _this2._publishedStreams = new Map(); // Key is streams published, value is its publication.

    _this2._pendingStreams = []; // Streams going to be added to PeerConnection.

    _this2._publishingStreams = []; // Streams have been added to PeerConnection, but does not receive ack from remote side.

    _this2._pendingUnpublishStreams = []; // Streams going to be removed.
    // Key is MediaStream's ID, value is an object {source:{audio:string, video:string}, attributes: object, stream: RemoteStream, mediaStream: MediaStream}. `stream` and `mediaStream` will be set when `track` event is fired on `RTCPeerConnection`. `mediaStream` will be `null` after `streamadded` event is fired on `P2PClient`. Other propertes will be set upon `STREAM_INFO` event from signaling channel.

    _this2._remoteStreamInfo = new Map();
    _this2._remoteStreams = [];
    _this2._remoteTrackSourceInfo = new Map(); // Key is MediaStreamTrack's ID, value is source info.

    _this2._publishPromises = new Map(); // Key is MediaStream's ID, value is an object has |resolve| and |reject|.

    _this2._unpublishPromises = new Map(); // Key is MediaStream's ID, value is an object has |resolve| and |reject|.

    _this2._publishingStreamTracks = new Map(); // Key is MediaStream's ID, value is an array of the ID of its MediaStreamTracks that haven't been acked.

    _this2._publishedStreamTracks = new Map(); // Key is MediaStream's ID, value is an array of the ID of its MediaStreamTracks that haven't been removed.

    _this2._isNegotiationNeeded = false;
    _this2._remoteSideSupportsRemoveStream = true;
    _this2._remoteSideIgnoresDataChannelAcks = false;
    _this2._remoteIceCandidates = [];
    _this2._dataChannels = new Map(); // Key is data channel's label, value is a RTCDataChannel.

    _this2._pendingMessages = [];
    _this2._dataSeq = 1; // Sequence number for data channel messages.

    _this2._sendDataPromises = new Map(); // Key is data sequence number, value is an object has |resolve| and |reject|.

    _this2._addedTrackIds = []; // Tracks that have been added after receiving remote SDP but before connection is established. Draining these messages when ICE connection state is connected.

    _this2._isPolitePeer = localId < remoteId;
    _this2._settingLocalSdp = false;
    _this2._settingRemoteSdp = false;
    _this2._disposed = false;

    _this2._createPeerConnection();

    _this2._sendUa(sysInfo);

    return _this2;
  }

  (0, _createClass2["default"])(P2PPeerConnectionChannel, [{
    key: "publish",

    /**
     * @function publish
     * @desc Publish a stream to the remote endpoint.
     * @private
     */
    value: function publish(stream) {
      var _this3 = this;

      if (!(stream instanceof StreamModule.LocalStream)) {
        return Promise.reject(new TypeError('Invalid stream.'));
      }

      if (this._publishedStreams.has(stream)) {
        return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_ILLEGAL_ARGUMENT, 'Duplicated stream.'));
      }

      if (this._areAllTracksEnded(stream.mediaStream)) {
        return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_INVALID_STATE, 'All tracks are ended.'));
      }

      return this._sendStreamInfo(stream).then(function () {
        return new Promise(function (resolve, reject) {
          _this3._addStream(stream.mediaStream);

          _this3._publishingStreams.push(stream);

          var trackIds = Array.from(stream.mediaStream.getTracks(), function (track) {
            return track.id;
          });

          _this3._publishingStreamTracks.set(stream.mediaStream.id, trackIds);

          _this3._publishPromises.set(stream.mediaStream.id, {
            resolve: resolve,
            reject: reject
          });
        });
      });
    }
    /**
     * @function send
     * @desc Send a message to the remote endpoint.
     * @private
     */

  }, {
    key: "send",
    value: function send(message) {
      var _this4 = this;

      if (!(typeof message === 'string')) {
        return Promise.reject(new TypeError('Invalid message.'));
      }

      var data = {
        id: this._dataSeq++,
        data: message
      };

      if (!this._dataChannels.has(DataChannelLabel.MESSAGE)) {
        this._createDataChannel(DataChannelLabel.MESSAGE);
      }

      var dc = this._dataChannels.get(DataChannelLabel.MESSAGE);

      if (dc.readyState === 'open') {
        this._dataChannels.get(DataChannelLabel.MESSAGE).send(JSON.stringify(data));

        return Promise.resolve();
      } else {
        this._pendingMessages.push(data);

        var promise = new Promise(function (resolve, reject) {
          _this4._sendDataPromises.set(data.id, {
            resolve: resolve,
            reject: reject
          });
        });
        return promise;
      }
    }
    /**
     * @function stop
     * @desc Stop the connection with remote endpoint.
     * @private
     */

  }, {
    key: "stop",
    value: function stop() {
      this._stop(undefined, true);
    }
    /**
     * @function getStats
     * @desc Get stats for a specific MediaStream.
     * @private
     */

  }, {
    key: "getStats",
    value: function getStats(mediaStream) {
      var _this5 = this;

      if (this._pc) {
        if (mediaStream === undefined) {
          return this._pc.getStats();
        } else {
          var tracksStatsReports = [];
          return Promise.all([mediaStream.getTracks().forEach(function (track) {
            _this5._getStats(track, tracksStatsReports);
          })]).then(function () {
            return new Promise(function (resolve, reject) {
              resolve(tracksStatsReports);
            });
          });
        }
      } else {
        return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_INVALID_STATE));
      }
    }
  }, {
    key: "_getStats",
    value: function _getStats(mediaStreamTrack, reportsResult) {
      return this._pc.getStats(mediaStreamTrack).then(function (statsReport) {
        reportsResult.push(statsReport);
      });
    }
    /**
     * @function _addStream
     * @desc Create RTCRtpSenders for all tracks in the stream.
     * @private
     */

  }, {
    key: "_addStream",
    value: function _addStream(stream) {
      var _iterator = _createForOfIteratorHelper(stream.getTracks()),
          _step;

      try {
        for (_iterator.s(); !(_step = _iterator.n()).done;) {
          var track = _step.value;

          this._pc.addTransceiver(track, {
            direction: 'sendonly',
            streams: [stream]
          });
        }
      } catch (err) {
        _iterator.e(err);
      } finally {
        _iterator.f();
      }
    }
    /**
     * @function onMessage
     * @desc This method is called by P2PClient when there is new signaling message arrived.
     * @private
     */

  }, {
    key: "onMessage",
    value: function onMessage(message) {
      this._SignalingMesssageHandler(message);
    }
  }, {
    key: "_sendSdp",
    value: function _sendSdp(sdp) {
      return this._signaling.sendSignalingMessage(this._remoteId, this._connectionId, SignalingType.SDP, sdp);
    }
  }, {
    key: "_sendUa",
    value: function _sendUa(sysInfo) {
      var ua = {
        sdk: sysInfo.sdk,
        capabilities: sysInfo.capabilities
      };

      this._sendSignalingMessage(SignalingType.UA, ua);
    }
  }, {
    key: "_sendSignalingMessage",
    value: function _sendSignalingMessage(type, message) {
      return this._signaling.sendSignalingMessage(this._remoteId, this._connectionId, type, message);
    }
  }, {
    key: "_SignalingMesssageHandler",
    value: function _SignalingMesssageHandler(message) {
      _logger["default"].debug('Channel received message: ' + message);

      switch (message.type) {
        case SignalingType.UA:
          this._handleRemoteCapability(message.data);

          break;

        case SignalingType.TRACK_SOURCES:
          this._trackSourcesHandler(message.data);

          break;

        case SignalingType.STREAM_INFO:
          this._streamInfoHandler(message.data);

          break;

        case SignalingType.SDP:
          this._sdpHandler(message.data);

          break;

        case SignalingType.TRACKS_ADDED:
          this._tracksAddedHandler(message.data);

          break;

        case SignalingType.TRACKS_REMOVED:
          this._tracksRemovedHandler(message.data);

          break;

        case SignalingType.DATA_RECEIVED:
          this._dataReceivedHandler(message.data);

          break;

        case SignalingType.CLOSED:
          this._chatClosedHandler(message.data);

          break;

        default:
          _logger["default"].error('Invalid signaling message received. Type: ' + message.type);

      }
    }
    /**
     * @function _tracksAddedHandler
     * @desc Handle track added event from remote side.
     * @private
     */

  }, {
    key: "_tracksAddedHandler",
    value: function _tracksAddedHandler(ids) {
      var _this6 = this;

      // Currently, |ids| contains all track IDs of a MediaStream. Following algorithm also handles |ids| is a part of a MediaStream's tracks.
      var _iterator2 = _createForOfIteratorHelper(ids),
          _step2;

      try {
        var _loop = function _loop() {
          var id = _step2.value;

          // It could be a problem if there is a track published with different MediaStreams, moving to mid.
          _this6._publishingStreamTracks.forEach(function (mediaTrackIds, mediaStreamId) {
            for (var i = 0; i < mediaTrackIds.length; i++) {
              if (mediaTrackIds[i] === id) {
                // Move this track from publishing tracks to published tracks.
                if (!_this6._publishedStreamTracks.has(mediaStreamId)) {
                  _this6._publishedStreamTracks.set(mediaStreamId, []);
                }

                _this6._publishedStreamTracks.get(mediaStreamId).push(mediaTrackIds[i]);

                mediaTrackIds.splice(i, 1);
              } // Resolving certain publish promise when remote endpoint received all tracks of a MediaStream.


              if (mediaTrackIds.length == 0) {
                var _ret = function () {
                  if (!_this6._publishPromises.has(mediaStreamId)) {
                    _logger["default"].warning('Cannot find the promise for publishing ' + mediaStreamId);

                    return "continue";
                  }

                  var targetStreamIndex = _this6._publishingStreams.findIndex(function (element) {
                    return element.mediaStream.id == mediaStreamId;
                  });

                  var targetStream = _this6._publishingStreams[targetStreamIndex];

                  _this6._publishingStreams.splice(targetStreamIndex, 1); // Set transceivers for Publication.


                  var transport = new _transport.TransportSettings(_transport.TransportType.WEBRTC, undefined);
                  transport.rtpTransceivers = [];

                  var _iterator3 = _createForOfIteratorHelper(_this6._pc.getTransceivers()),
                      _step3;

                  try {
                    for (_iterator3.s(); !(_step3 = _iterator3.n()).done;) {
                      var _transceiver$sender;

                      var transceiver = _step3.value;

                      if (((_transceiver$sender = transceiver.sender) === null || _transceiver$sender === void 0 ? void 0 : _transceiver$sender.track) in _this6._publishedStreamTracks.get(mediaStreamId)) {
                        transport.rtpTransceivers.push(transceiver);
                      }
                    }
                  } catch (err) {
                    _iterator3.e(err);
                  } finally {
                    _iterator3.f();
                  }

                  var publication = new _publication.Publication(id, transport, function () {
                    _this6._unpublish(targetStream).then(function () {
                      publication.dispatchEvent(new _event.OwtEvent('ended'));
                    }, function (err) {
                      // Use debug mode because this error usually doesn't block stopping a publication.
                      _logger["default"].debug('Something wrong happened during stopping a ' + 'publication. ' + err.message);
                    });
                  }, function () {
                    if (!targetStream || !targetStream.mediaStream) {
                      return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_INVALID_STATE, 'Publication is not available.'));
                    }

                    return _this6.getStats(targetStream.mediaStream);
                  });

                  _this6._publishedStreams.set(targetStream, publication);

                  _this6._publishPromises.get(mediaStreamId).resolve(publication);

                  _this6._publishPromises["delete"](mediaStreamId);
                }();

                if (_ret === "continue") continue;
              }
            }
          });
        };

        for (_iterator2.s(); !(_step2 = _iterator2.n()).done;) {
          _loop();
        }
      } catch (err) {
        _iterator2.e(err);
      } finally {
        _iterator2.f();
      }
    }
    /**
     * @function _tracksRemovedHandler
     * @desc Handle track removed event from remote side.
     * @private
     */

  }, {
    key: "_tracksRemovedHandler",
    value: function _tracksRemovedHandler(ids) {
      var _this7 = this;

      // Currently, |ids| contains all track IDs of a MediaStream. Following algorithm also handles |ids| is a part of a MediaStream's tracks.
      var _iterator4 = _createForOfIteratorHelper(ids),
          _step4;

      try {
        var _loop2 = function _loop2() {
          var id = _step4.value;

          // It could be a problem if there is a track published with different MediaStreams.
          _this7._publishedStreamTracks.forEach(function (mediaTrackIds, mediaStreamId) {
            for (var i = 0; i < mediaTrackIds.length; i++) {
              if (mediaTrackIds[i] === id) {
                mediaTrackIds.splice(i, 1);
              }
            }
          });
        };

        for (_iterator4.s(); !(_step4 = _iterator4.n()).done;) {
          _loop2();
        }
      } catch (err) {
        _iterator4.e(err);
      } finally {
        _iterator4.f();
      }
    }
    /**
     * @function _dataReceivedHandler
     * @desc Handle data received event from remote side.
     * @private
     */

  }, {
    key: "_dataReceivedHandler",
    value: function _dataReceivedHandler(id) {
      if (!this._sendDataPromises.has(id)) {
        _logger["default"].warning('Received unknown data received message. ID: ' + id);

        return;
      } else {
        this._sendDataPromises.get(id).resolve();
      }
    }
    /**
     * @function _sdpHandler
     * @desc Handle SDP received event from remote side.
     * @private
     */

  }, {
    key: "_sdpHandler",
    value: function _sdpHandler(sdp) {
      if (sdp.type === 'offer') {
        this._onOffer(sdp);
      } else if (sdp.type === 'answer') {
        this._onAnswer(sdp);
      } else if (sdp.type === 'candidates') {
        this._onRemoteIceCandidate(sdp);
      }
    }
    /**
     * @function _trackSourcesHandler
     * @desc Received track source information from remote side.
     * @private
     */

  }, {
    key: "_trackSourcesHandler",
    value: function _trackSourcesHandler(data) {
      var _iterator5 = _createForOfIteratorHelper(data),
          _step5;

      try {
        for (_iterator5.s(); !(_step5 = _iterator5.n()).done;) {
          var info = _step5.value;

          this._remoteTrackSourceInfo.set(info.id, info.source);
        }
      } catch (err) {
        _iterator5.e(err);
      } finally {
        _iterator5.f();
      }
    }
    /**
     * @function _streamInfoHandler
     * @desc Received stream information from remote side.
     * @private
     */

  }, {
    key: "_streamInfoHandler",
    value: function _streamInfoHandler(data) {
      if (!data) {
        _logger["default"].warning('Unexpected stream info.');

        return;
      }

      this._remoteStreamInfo.set(data.id, {
        source: data.source,
        attributes: data.attributes,
        stream: null,
        mediaStream: null,
        trackIds: data.tracks // Track IDs may not match at sender and receiver sides. Keep it for legacy porposes.

      });
    }
    /**
     * @function _chatClosedHandler
     * @desc Received chat closed event from remote side.
     * @private
     */

  }, {
    key: "_chatClosedHandler",
    value: function _chatClosedHandler(data) {
      this._disposed = true;

      this._stop(data, false);
    }
  }, {
    key: "_onOffer",
    value: function _onOffer(sdp) {
      var _this8 = this;

      _logger["default"].debug('About to set remote description. Signaling state: ' + this._pc.signalingState);

      if (this._pc.signalingState !== 'stable' || this._settingLocalSdp) {
        if (this._isPolitePeer) {
          _logger["default"].debug('Rollback.');

          this._settingLocalSdp = true; // setLocalDescription(rollback) is not supported on Safari right now.
          // Test case "WebRTC collision should be resolved." is expected to fail.
          // See
          // https://wpt.fyi/results/webrtc/RTCPeerConnection-setLocalDescription-rollback.html?q=webrtc&run_id=5662062321598464&run_id=5756139520131072&run_id=5754637556645888&run_id=5764334049296384.

          this._pc.setLocalDescription().then(function () {
            _this8._settingLocalSdp = false;
          });
        } else {
          _logger["default"].debug('Collision detected. Ignore this offer.');

          return;
        }
      }

      sdp.sdp = this._setRtpSenderOptions(sdp.sdp, this._config);
      var sessionDescription = new RTCSessionDescription(sdp);
      this._settingRemoteSdp = true;

      this._pc.setRemoteDescription(sessionDescription).then(function () {
        _this8._settingRemoteSdp = false;

        _this8._createAndSendAnswer();
      }, function (error) {
        _logger["default"].debug('Set remote description failed. Message: ' + error.message);

        _this8._stop(error, true);
      });
    }
  }, {
    key: "_onAnswer",
    value: function _onAnswer(sdp) {
      var _this9 = this;

      _logger["default"].debug('About to set remote description. Signaling state: ' + this._pc.signalingState);

      sdp.sdp = this._setRtpSenderOptions(sdp.sdp, this._config);
      var sessionDescription = new RTCSessionDescription(sdp);
      this._settingRemoteSdp = true;

      this._pc.setRemoteDescription(new RTCSessionDescription(sessionDescription)).then(function () {
        _logger["default"].debug('Set remote descripiton successfully.');

        _this9._settingRemoteSdp = false;

        _this9._drainPendingMessages();
      }, function (error) {
        _logger["default"].debug('Set remote description failed. Message: ' + error.message);

        _this9._stop(error, true);
      });
    }
  }, {
    key: "_onLocalIceCandidate",
    value: function _onLocalIceCandidate(event) {
      if (event.candidate) {
        this._sendSdp({
          type: 'candidates',
          candidate: event.candidate.candidate,
          sdpMid: event.candidate.sdpMid,
          sdpMLineIndex: event.candidate.sdpMLineIndex
        })["catch"](function (e) {
          _logger["default"].warning('Failed to send candidate.');
        });
      } else {
        _logger["default"].debug('Empty candidate.');
      }
    }
  }, {
    key: "_onRemoteTrackAdded",
    value: function _onRemoteTrackAdded(event) {
      _logger["default"].debug('Remote track added.');

      var _iterator6 = _createForOfIteratorHelper(event.streams),
          _step6;

      try {
        for (_iterator6.s(); !(_step6 = _iterator6.n()).done;) {
          var stream = _step6.value;

          if (!this._remoteStreamInfo.has(stream.id)) {
            _logger["default"].warning('Missing stream info.');

            return;
          }

          if (!this._remoteStreamInfo.get(stream.id).stream) {
            this._setStreamToRemoteStreamInfo(stream);
          }
        }
      } catch (err) {
        _iterator6.e(err);
      } finally {
        _iterator6.f();
      }

      if (this._pc.iceConnectionState === 'connected' || this._pc.iceConnectionState === 'completed') {
        this._checkIceConnectionStateAndFireEvent();
      } else {
        this._addedTrackIds.concat(event.track.id);
      }
    }
  }, {
    key: "_onRemoteStreamAdded",
    value: function _onRemoteStreamAdded(event) {
      _logger["default"].debug('Remote stream added.');

      if (!this._remoteStreamInfo.has(event.stream.id)) {
        _logger["default"].warning('Cannot find source info for stream ' + event.stream.id);

        return;
      }

      if (this._pc.iceConnectionState === 'connected' || this._pc.iceConnectionState === 'completed') {
        this._sendSignalingMessage(SignalingType.TRACKS_ADDED, this._remoteStreamInfo.get(event.stream.id).trackIds);
      } else {
        this._addedTrackIds = this._addedTrackIds.concat(this._remoteStreamInfo.get(event.stream.id).trackIds);
      }

      var audioTrackSource = this._remoteStreamInfo.get(event.stream.id).source.audio;

      var videoTrackSource = this._remoteStreamInfo.get(event.stream.id).source.video;

      var sourceInfo = new StreamModule.StreamSourceInfo(audioTrackSource, videoTrackSource);

      if (Utils.isSafari()) {
        if (!sourceInfo.audio) {
          event.stream.getAudioTracks().forEach(function (track) {
            event.stream.removeTrack(track);
          });
        }

        if (!sourceInfo.video) {
          event.stream.getVideoTracks().forEach(function (track) {
            event.stream.removeTrack(track);
          });
        }
      }

      var attributes = this._remoteStreamInfo.get(event.stream.id).attributes;

      var stream = new StreamModule.RemoteStream(undefined, this._remoteId, event.stream, sourceInfo, attributes);

      if (stream) {
        this._remoteStreams.push(stream);

        var streamEvent = new StreamModule.StreamEvent('streamadded', {
          stream: stream
        });
        this.dispatchEvent(streamEvent);
      }
    }
  }, {
    key: "_onRemoteStreamRemoved",
    value: function _onRemoteStreamRemoved(event) {
      _logger["default"].debug('Remote stream removed.');

      var i = this._remoteStreams.findIndex(function (s) {
        return s.mediaStream.id === event.stream.id;
      });

      if (i !== -1) {
        var stream = this._remoteStreams[i];

        this._streamRemoved(stream);

        this._remoteStreams.splice(i, 1);
      }
    }
  }, {
    key: "_onNegotiationneeded",
    value: function _onNegotiationneeded() {
      if (this._pc.signalingState === 'stable' && !this._pc._settingLocalSdp && !this._settingRemoteSdp) {
        this._doNegotiate();
      } else {
        this._isNegotiationNeeded = true;
      }
    }
  }, {
    key: "_onRemoteIceCandidate",
    value: function _onRemoteIceCandidate(candidateInfo) {
      var candidate = new RTCIceCandidate({
        candidate: candidateInfo.candidate,
        sdpMid: candidateInfo.sdpMid,
        sdpMLineIndex: candidateInfo.sdpMLineIndex
      });

      if (this._pc.remoteDescription && this._pc.remoteDescription.sdp !== '') {
        _logger["default"].debug('Add remote ice candidates.');

        this._pc.addIceCandidate(candidate)["catch"](function (error) {
          _logger["default"].warning('Error processing ICE candidate: ' + error);
        });
      } else {
        _logger["default"].debug('Cache remote ice candidates.');

        this._remoteIceCandidates.push(candidate);
      }
    }
  }, {
    key: "_onSignalingStateChange",
    value: function _onSignalingStateChange(event) {
      _logger["default"].debug('Signaling state changed: ' + this._pc.signalingState);

      if (this._pc.signalingState === 'have-remote-offer' || this._pc.signalingState === 'stable') {
        this._drainPendingRemoteIceCandidates();
      }

      if (this._pc.signalingState === 'stable') {
        this._negotiating = false;

        if (this._isNegotiationNeeded) {
          this._onNegotiationneeded();
        } else {
          this._drainPendingStreams();

          this._drainPendingMessages();
        }
      }
    }
  }, {
    key: "_onIceConnectionStateChange",
    value: function _onIceConnectionStateChange(event) {
      if (event.currentTarget.iceConnectionState === 'closed' || event.currentTarget.iceConnectionState === 'failed') {
        var error = new ErrorModule.P2PError(ErrorModule.errors.P2P_WEBRTC_UNKNOWN, 'ICE connection failed or closed.');

        this._stop(error, true);
      } else if (event.currentTarget.iceConnectionState === 'connected' || event.currentTarget.iceConnectionState === 'completed') {
        this._sendSignalingMessage(SignalingType.TRACKS_ADDED, this._addedTrackIds);

        this._addedTrackIds = [];

        this._checkIceConnectionStateAndFireEvent();
      }
    }
  }, {
    key: "_onDataChannelMessage",
    value: function _onDataChannelMessage(event) {
      var message = JSON.parse(event.data);

      if (!this._remoteSideIgnoresDataChannelAcks) {
        this._sendSignalingMessage(SignalingType.DATA_RECEIVED, message.id);
      }

      var messageEvent = new _event.MessageEvent('messagereceived', {
        message: message.data,
        origin: this._remoteId
      });
      this.dispatchEvent(messageEvent);
    }
  }, {
    key: "_onDataChannelOpen",
    value: function _onDataChannelOpen(event) {
      _logger["default"].debug('Data Channel is opened.');

      if (event.target.label === DataChannelLabel.MESSAGE) {
        _logger["default"].debug('Data channel for messages is opened.');

        this._drainPendingMessages();
      }
    }
  }, {
    key: "_onDataChannelClose",
    value: function _onDataChannelClose(event) {
      _logger["default"].debug('Data Channel is closed.');
    }
  }, {
    key: "_streamRemoved",
    value: function _streamRemoved(stream) {
      if (!this._remoteStreamInfo.has(stream.mediaStream.id)) {
        _logger["default"].warning('Cannot find stream info.');
      }

      this._sendSignalingMessage(SignalingType.TRACKS_REMOVED, this._remoteStreamInfo.get(stream.mediaStream.id).trackIds);

      var event = new _event.OwtEvent('ended');
      stream.dispatchEvent(event);
    }
  }, {
    key: "_createPeerConnection",
    value: function _createPeerConnection() {
      var _this10 = this;

      var pcConfiguration = this._config.rtcConfiguration || {};
      this._pc = new RTCPeerConnection(pcConfiguration);

      this._pc.ontrack = function (event) {
        _this10._onRemoteTrackAdded.apply(_this10, [event]);
      };

      this._pc.onicecandidate = function (event) {
        _this10._onLocalIceCandidate.apply(_this10, [event]);
      };

      this._pc.onsignalingstatechange = function (event) {
        _this10._onSignalingStateChange.apply(_this10, [event]);
      };

      this._pc.ondatachannel = function (event) {
        _logger["default"].debug('On data channel.'); // Save remote created data channel.


        if (!_this10._dataChannels.has(event.channel.label)) {
          _this10._dataChannels.set(event.channel.label, event.channel);

          _logger["default"].debug('Save remote created data channel.');
        }

        _this10._bindEventsToDataChannel(event.channel);
      };

      this._pc.oniceconnectionstatechange = function (event) {
        _this10._onIceConnectionStateChange.apply(_this10, [event]);
      };

      this._pc.onnegotiationneeded = function () {
        _this10._onNegotiationneeded();
      };
    }
  }, {
    key: "_drainPendingStreams",
    value: function _drainPendingStreams() {
      _logger["default"].debug('Draining pending streams.');

      if (this._pc && this._pc.signalingState === 'stable') {
        _logger["default"].debug('Peer connection is ready for draining pending streams.');

        for (var i = 0; i < this._pendingStreams.length; i++) {
          var stream = this._pendingStreams[i];

          this._pendingStreams.shift();

          if (!stream.mediaStream) {
            continue;
          }

          this._addStream(stream.mediaStream);

          _logger["default"].debug('Added stream to peer connection.');

          this._publishingStreams.push(stream);
        }

        this._pendingStreams.length = 0;

        var _iterator7 = _createForOfIteratorHelper(this._pendingUnpublishStreams),
            _step7;

        try {
          for (_iterator7.s(); !(_step7 = _iterator7.n()).done;) {
            var _stream = _step7.value;

            if (!_stream.stream) {
              continue;
            }

            if (typeof this._pc.getTransceivers === 'function' && typeof this._pc.removeTrack === 'function') {
              var _iterator8 = _createForOfIteratorHelper(this._pc.getTransceivers()),
                  _step8;

              try {
                for (_iterator8.s(); !(_step8 = _iterator8.n()).done;) {
                  var transceiver = _step8.value;

                  var _iterator9 = _createForOfIteratorHelper(_stream.stream.getTracks()),
                      _step9;

                  try {
                    for (_iterator9.s(); !(_step9 = _iterator9.n()).done;) {
                      var track = _step9.value;

                      if (transceiver.sender.track == track) {
                        if (transceiver.direction === 'sendonly') {
                          transceiver.stop();
                        } else {
                          this._pc.removeTrack(track);
                        }
                      }
                    }
                  } catch (err) {
                    _iterator9.e(err);
                  } finally {
                    _iterator9.f();
                  }
                }
              } catch (err) {
                _iterator8.e(err);
              } finally {
                _iterator8.f();
              }
            } else {
              _logger["default"].debug('getSender or removeTrack is not supported, fallback to ' + 'removeStream.');

              this._pc.removeStream(_stream.stream);
            }

            this._unpublishPromises.get(_stream.stream.id).resolve();

            this._publishedStreams["delete"](_stream);
          }
        } catch (err) {
          _iterator7.e(err);
        } finally {
          _iterator7.f();
        }

        this._pendingUnpublishStreams.length = 0;
      }
    }
  }, {
    key: "_drainPendingRemoteIceCandidates",
    value: function _drainPendingRemoteIceCandidates() {
      for (var i = 0; i < this._remoteIceCandidates.length; i++) {
        _logger["default"].debug('Add candidate');

        this._pc.addIceCandidate(this._remoteIceCandidates[i])["catch"](function (error) {
          _logger["default"].warning('Error processing ICE candidate: ' + error);
        });
      }

      this._remoteIceCandidates.length = 0;
    }
  }, {
    key: "_drainPendingMessages",
    value: function _drainPendingMessages() {
      _logger["default"].debug('Draining pending messages.');

      if (this._pendingMessages.length == 0) {
        return;
      }

      var dc = this._dataChannels.get(DataChannelLabel.MESSAGE);

      if (dc && dc.readyState === 'open') {
        for (var i = 0; i < this._pendingMessages.length; i++) {
          _logger["default"].debug('Sending message via data channel: ' + this._pendingMessages[i]);

          dc.send(JSON.stringify(this._pendingMessages[i]));
          var messageId = this._pendingMessages[i].id;

          if (this._sendDataPromises.has(messageId)) {
            this._sendDataPromises.get(messageId).resolve();
          }
        }

        this._pendingMessages.length = 0;
      } else if (this._pc && this._pc.connectionState !== 'closed' && !dc) {
        this._createDataChannel(DataChannelLabel.MESSAGE);
      }
    }
  }, {
    key: "_sendStreamInfo",
    value: function _sendStreamInfo(stream) {
      if (!stream || !stream.mediaStream) {
        return new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_ILLEGAL_ARGUMENT);
      }

      var info = [];
      stream.mediaStream.getTracks().map(function (track) {
        info.push({
          id: track.id,
          source: stream.source[track.kind]
        });
      });
      return Promise.all([this._sendSignalingMessage(SignalingType.TRACK_SOURCES, info), this._sendSignalingMessage(SignalingType.STREAM_INFO, {
        id: stream.mediaStream.id,
        attributes: stream.attributes,
        // Track IDs may not match at sender and receiver sides.
        tracks: Array.from(info, function (item) {
          return item.id;
        }),
        // This is a workaround for Safari. Please use track-sources if possible.
        source: stream.source
      })]);
    }
  }, {
    key: "_handleRemoteCapability",
    value: function _handleRemoteCapability(ua) {
      if (ua.sdk && ua.sdk && ua.sdk.type === 'JavaScript' && ua.runtime && ua.runtime.name === 'Firefox') {
        this._remoteSideSupportsRemoveStream = false;
      } else {
        // Remote side is iOS/Android/C++ which uses Google's WebRTC stack.
        this._remoteSideSupportsRemoveStream = true;
      }

      if (ua.capabilities) {
        this._remoteSideIgnoresDataChannelAcks = ua.capabilities.ignoreDataChannelAcks;
      }
    }
  }, {
    key: "_doNegotiate",
    value: function _doNegotiate() {
      this._createAndSendOffer();
    }
  }, {
    key: "_setCodecOrder",
    value: function _setCodecOrder(sdp) {
      if (this._config.audioEncodings) {
        var audioCodecNames = Array.from(this._config.audioEncodings, function (encodingParameters) {
          return encodingParameters.codec.name;
        });
        sdp = SdpUtils.reorderCodecs(sdp, 'audio', audioCodecNames);
      }

      if (this._config.videoEncodings) {
        var videoCodecNames = Array.from(this._config.videoEncodings, function (encodingParameters) {
          return encodingParameters.codec.name;
        });
        sdp = SdpUtils.reorderCodecs(sdp, 'video', videoCodecNames);
      }

      return sdp;
    }
  }, {
    key: "_setMaxBitrate",
    value: function _setMaxBitrate(sdp, options) {
      if ((0, _typeof2["default"])(options.audioEncodings) === 'object') {
        sdp = SdpUtils.setMaxBitrate(sdp, options.audioEncodings);
      }

      if ((0, _typeof2["default"])(options.videoEncodings) === 'object') {
        sdp = SdpUtils.setMaxBitrate(sdp, options.videoEncodings);
      }

      return sdp;
    }
  }, {
    key: "_setRtpSenderOptions",
    value: function _setRtpSenderOptions(sdp, options) {
      sdp = this._setMaxBitrate(sdp, options);
      return sdp;
    }
  }, {
    key: "_setRtpReceiverOptions",
    value: function _setRtpReceiverOptions(sdp) {
      sdp = this._setCodecOrder(sdp);
      return sdp;
    }
  }, {
    key: "_createAndSendOffer",
    value: function _createAndSendOffer() {
      var _this11 = this;

      if (!this._pc) {
        _logger["default"].error('Peer connection have not been created.');

        return;
      }

      this._isNegotiationNeeded = false;

      this._pc.createOffer().then(function (desc) {
        desc.sdp = _this11._setRtpReceiverOptions(desc.sdp);

        if (_this11._pc.signalingState === 'stable' && !_this11._settingLocalSdp && !_this11._settingRemoteSdp) {
          _this11._settingLocalSdp = true;
          return _this11._pc.setLocalDescription(desc).then(function () {
            _this11._settingLocalSdp = false;
            return _this11._sendSdp(_this11._pc.localDescription);
          });
        }
      })["catch"](function (e) {
        _logger["default"].error(e.message);

        var error = new ErrorModule.P2PError(ErrorModule.errors.P2P_WEBRTC_SDP, e.message);

        _this11._stop(error, true);
      });
    }
  }, {
    key: "_createAndSendAnswer",
    value: function _createAndSendAnswer() {
      var _this12 = this;

      this._drainPendingStreams();

      this._isNegotiationNeeded = false;

      this._pc.createAnswer().then(function (desc) {
        desc.sdp = _this12._setRtpReceiverOptions(desc.sdp);

        _this12._logCurrentAndPendingLocalDescription();

        _this12._settingLocalSdp = true;
        return _this12._pc.setLocalDescription(desc).then(function () {
          _this12._settingLocalSdp = false;
        });
      }).then(function () {
        return _this12._sendSdp(_this12._pc.localDescription);
      })["catch"](function (e) {
        _logger["default"].error(e.message + ' Please check your codec settings.');

        var error = new ErrorModule.P2PError(ErrorModule.errors.P2P_WEBRTC_SDP, e.message);

        _this12._stop(error, true);
      });
    }
  }, {
    key: "_logCurrentAndPendingLocalDescription",
    value: function _logCurrentAndPendingLocalDescription() {
      _logger["default"].info('Current description: ' + this._pc.currentLocalDescription);

      _logger["default"].info('Pending description: ' + this._pc.pendingLocalDescription);
    }
  }, {
    key: "_getAndDeleteTrackSourceInfo",
    value: function _getAndDeleteTrackSourceInfo(tracks) {
      if (tracks.length > 0) {
        var trackId = tracks[0].id;

        if (this._remoteTrackSourceInfo.has(trackId)) {
          var sourceInfo = this._remoteTrackSourceInfo.get(trackId);

          this._remoteTrackSourceInfo["delete"](trackId);

          return sourceInfo;
        } else {
          _logger["default"].warning('Cannot find source info for ' + trackId);
        }
      }
    }
  }, {
    key: "_unpublish",
    value: function _unpublish(stream) {
      var _this13 = this;

      if (navigator.mozGetUserMedia || !this._remoteSideSupportsRemoveStream) {
        // Actually unpublish is supported. It is a little bit complex since
        // Firefox implemented WebRTC spec while Chrome implemented an old API.
        _logger["default"].error('Stopping a publication is not supported on Firefox. Please use ' + 'P2PClient.stop() to stop the connection with remote endpoint.');

        return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_UNSUPPORTED_METHOD));
      }

      if (!this._publishedStreams.has(stream)) {
        return Promise.reject(new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_ILLEGAL_ARGUMENT));
      }

      this._pendingUnpublishStreams.push(stream);

      return new Promise(function (resolve, reject) {
        _this13._unpublishPromises.set(stream.mediaStream.id, {
          resolve: resolve,
          reject: reject
        });

        _this13._drainPendingStreams();
      });
    } // Make sure |_pc| is available before calling this method.

  }, {
    key: "_createDataChannel",
    value: function _createDataChannel(label) {
      if (this._dataChannels.has(label)) {
        _logger["default"].warning('Data channel labeled ' + label + ' already exists.');

        return;
      }

      if (!this._pc) {
        _logger["default"].debug('PeerConnection is not available before creating DataChannel.');

        return;
      }

      _logger["default"].debug('Create data channel.');

      var dc = this._pc.createDataChannel(label);

      this._bindEventsToDataChannel(dc);

      this._dataChannels.set(DataChannelLabel.MESSAGE, dc);
    }
  }, {
    key: "_bindEventsToDataChannel",
    value: function _bindEventsToDataChannel(dc) {
      var _this14 = this;

      dc.onmessage = function (event) {
        _this14._onDataChannelMessage.apply(_this14, [event]);
      };

      dc.onopen = function (event) {
        _this14._onDataChannelOpen.apply(_this14, [event]);
      };

      dc.onclose = function (event) {
        _this14._onDataChannelClose.apply(_this14, [event]);
      };

      dc.onerror = function (event) {
        _logger["default"].debug('Data Channel Error: ' + event);
      };
    } // Returns all MediaStreams it belongs to.

  }, {
    key: "_getStreamByTrack",
    value: function _getStreamByTrack(mediaStreamTrack) {
      var streams = [];

      var _iterator10 = _createForOfIteratorHelper(this._remoteStreamInfo),
          _step10;

      try {
        for (_iterator10.s(); !(_step10 = _iterator10.n()).done;) {
          var _step10$value = (0, _slicedToArray2["default"])(_step10.value, 2),

          /* id */
          info = _step10$value[1];

          if (!info.stream || !info.stream.mediaStream) {
            continue;
          }

          var _iterator11 = _createForOfIteratorHelper(info.stream.mediaStream.getTracks()),
              _step11;

          try {
            for (_iterator11.s(); !(_step11 = _iterator11.n()).done;) {
              var track = _step11.value;

              if (mediaStreamTrack === track) {
                streams.push(info.stream.mediaStream);
              }
            }
          } catch (err) {
            _iterator11.e(err);
          } finally {
            _iterator11.f();
          }
        }
      } catch (err) {
        _iterator10.e(err);
      } finally {
        _iterator10.f();
      }

      return streams;
    }
  }, {
    key: "_areAllTracksEnded",
    value: function _areAllTracksEnded(mediaStream) {
      var _iterator12 = _createForOfIteratorHelper(mediaStream.getTracks()),
          _step12;

      try {
        for (_iterator12.s(); !(_step12 = _iterator12.n()).done;) {
          var track = _step12.value;

          if (track.readyState === 'live') {
            return false;
          }
        }
      } catch (err) {
        _iterator12.e(err);
      } finally {
        _iterator12.f();
      }

      return true;
    }
  }, {
    key: "_stop",
    value: function _stop(error, notifyRemote) {
      var promiseError = error;

      if (!promiseError) {
        promiseError = new ErrorModule.P2PError(ErrorModule.errors.P2P_CLIENT_UNKNOWN);
      }

      var _iterator13 = _createForOfIteratorHelper(this._dataChannels),
          _step13;

      try {
        for (_iterator13.s(); !(_step13 = _iterator13.n()).done;) {
          var _step13$value = (0, _slicedToArray2["default"])(_step13.value, 2),

          /* label */
          dc = _step13$value[1];

          dc.close();
        }
      } catch (err) {
        _iterator13.e(err);
      } finally {
        _iterator13.f();
      }

      this._dataChannels.clear();

      if (this._pc && this._pc.iceConnectionState !== 'closed') {
        this._pc.close();
      }

      var _iterator14 = _createForOfIteratorHelper(this._publishPromises),
          _step14;

      try {
        for (_iterator14.s(); !(_step14 = _iterator14.n()).done;) {
          var _step14$value = (0, _slicedToArray2["default"])(_step14.value, 2),

          /* id */
          promise = _step14$value[1];

          promise.reject(promiseError);
        }
      } catch (err) {
        _iterator14.e(err);
      } finally {
        _iterator14.f();
      }

      this._publishPromises.clear();

      var _iterator15 = _createForOfIteratorHelper(this._unpublishPromises),
          _step15;

      try {
        for (_iterator15.s(); !(_step15 = _iterator15.n()).done;) {
          var _step15$value = (0, _slicedToArray2["default"])(_step15.value, 2),

          /* id */
          _promise = _step15$value[1];

          _promise.reject(promiseError);
        }
      } catch (err) {
        _iterator15.e(err);
      } finally {
        _iterator15.f();
      }

      this._unpublishPromises.clear();

      var _iterator16 = _createForOfIteratorHelper(this._sendDataPromises),
          _step16;

      try {
        for (_iterator16.s(); !(_step16 = _iterator16.n()).done;) {
          var _step16$value = (0, _slicedToArray2["default"])(_step16.value, 2),

          /* id */
          _promise2 = _step16$value[1];

          _promise2.reject(promiseError);
        }
      } catch (err) {
        _iterator16.e(err);
      } finally {
        _iterator16.f();
      }

      this._sendDataPromises.clear(); // Fire ended event if publication or remote stream exists.


      this._publishedStreams.forEach(function (publication) {
        publication.dispatchEvent(new _event.OwtEvent('ended'));
      });

      this._publishedStreams.clear();

      this._remoteStreams.forEach(function (stream) {
        stream.dispatchEvent(new _event.OwtEvent('ended'));
      });

      this._remoteStreams = [];

      if (!this._disposed) {
        if (notifyRemote) {
          var sendError;

          if (error) {
            sendError = JSON.parse(JSON.stringify(error)); // Avoid to leak detailed error to remote side.

            sendError.message = 'Error happened at remote side.';
          }

          this._sendSignalingMessage(SignalingType.CLOSED, sendError)["catch"](function (err) {
            _logger["default"].debug('Failed to send close.' + err.message);
          });
        }

        this.dispatchEvent(new Event('ended'));
      }
    }
  }, {
    key: "_setStreamToRemoteStreamInfo",
    value: function _setStreamToRemoteStreamInfo(mediaStream) {
      var info = this._remoteStreamInfo.get(mediaStream.id);

      var attributes = info.attributes;
      var sourceInfo = new StreamModule.StreamSourceInfo(this._remoteStreamInfo.get(mediaStream.id).source.audio, this._remoteStreamInfo.get(mediaStream.id).source.video);
      info.stream = new StreamModule.RemoteStream(undefined, this._remoteId, mediaStream, sourceInfo, attributes);
      info.mediaStream = mediaStream;
      var stream = info.stream;

      if (stream) {
        this._remoteStreams.push(stream);
      } else {
        _logger["default"].warning('Failed to create RemoteStream.');
      }
    }
  }, {
    key: "_checkIceConnectionStateAndFireEvent",
    value: function _checkIceConnectionStateAndFireEvent() {
      var _this15 = this;

      if (this._pc.iceConnectionState === 'connected' || this._pc.iceConnectionState === 'completed') {
        var _iterator17 = _createForOfIteratorHelper(this._remoteStreamInfo),
            _step17;

        try {
          for (_iterator17.s(); !(_step17 = _iterator17.n()).done;) {
            var _step17$value = (0, _slicedToArray2["default"])(_step17.value, 2),

            /* id */
            info = _step17$value[1];

            if (info.mediaStream) {
              var streamEvent = new StreamModule.StreamEvent('streamadded', {
                stream: info.stream
              });

              var _iterator18 = _createForOfIteratorHelper(info.mediaStream.getTracks()),
                  _step18;

              try {
                for (_iterator18.s(); !(_step18 = _iterator18.n()).done;) {
                  var track = _step18.value;
                  track.addEventListener('ended', function (event) {
                    var mediaStreams = _this15._getStreamByTrack(event.target);

                    var _iterator19 = _createForOfIteratorHelper(mediaStreams),
                        _step19;

                    try {
                      for (_iterator19.s(); !(_step19 = _iterator19.n()).done;) {
                        var mediaStream = _step19.value;

                        if (_this15._areAllTracksEnded(mediaStream)) {
                          _this15._onRemoteStreamRemoved({
                            stream: mediaStream
                          });
                        }
                      }
                    } catch (err) {
                      _iterator19.e(err);
                    } finally {
                      _iterator19.f();
                    }
                  });
                }
              } catch (err) {
                _iterator18.e(err);
              } finally {
                _iterator18.f();
              }

              this._sendSignalingMessage(SignalingType.TRACKS_ADDED, info.trackIds);

              this._remoteStreamInfo.get(info.mediaStream.id).mediaStream = null;
              this.dispatchEvent(streamEvent);
            }
          }
        } catch (err) {
          _iterator17.e(err);
        } finally {
          _iterator17.f();
        }
      }
    }
  }, {
    key: "peerConnection",
    get: function get() {
      return this._pc;
    }
  }]);
  return P2PPeerConnectionChannel;
}(_event.EventDispatcher);

var _default = P2PPeerConnectionChannel;
exports["default"] = _default;

},{"../base/event.js":26,"../base/logger.js":28,"../base/publication.js":31,"../base/sdputils.js":32,"../base/stream.js":33,"../base/transport.js":34,"../base/utils.js":35,"./error.js":48,"@babel/runtime/helpers/classCallCheck":5,"@babel/runtime/helpers/createClass":7,"@babel/runtime/helpers/getPrototypeOf":8,"@babel/runtime/helpers/inherits":9,"@babel/runtime/helpers/interopRequireDefault":10,"@babel/runtime/helpers/interopRequireWildcard":11,"@babel/runtime/helpers/possibleConstructorReturn":16,"@babel/runtime/helpers/slicedToArray":18,"@babel/runtime/helpers/typeof":19,"@babel/runtime/helpers/wrapNativeSuper":21}]},{},[47])(47)
});

//# sourceMappingURL=data:application/json;charset=utf-8;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIm5vZGVfbW9kdWxlcy9icm93c2VyLXBhY2svX3ByZWx1ZGUuanMiLCJub2RlX21vZHVsZXMvQGJhYmVsL3J1bnRpbWUvaGVscGVycy9hcnJheUxpa2VUb0FycmF5LmpzIiwibm9kZV9tb2R1bGVzL0BiYWJlbC9ydW50aW1lL2hlbHBlcnMvYXJyYXlXaXRoSG9sZXMuanMiLCJub2RlX21vZHVsZXMvQGJhYmVsL3J1bnRpbWUvaGVscGVycy9hc3NlcnRUaGlzSW5pdGlhbGl6ZWQuanMiLCJub2RlX21vZHVsZXMvQGJhYmVsL3J1bnRpbWUvaGVscGVycy9hc3luY1RvR2VuZXJhdG9yLmpzIiwibm9kZV9tb2R1bGVzL0BiYWJlbC9ydW50aW1lL2hlbHBlcnMvY2xhc3NDYWxsQ2hlY2suanMiLCJub2RlX21vZHVsZXMvQGJhYmVsL3J1bnRpbWUvaGVscGVycy9jb25zdHJ1Y3QuanMiLCJub2RlX21vZHVsZXMvQGJhYmVsL3J1bnRpbWUvaGVscGVycy9jcmVhdGVDbGFzcy5qcyIsIm5vZGVfbW9kdWxlcy9AYmFiZWwvcnVudGltZS9oZWxwZXJzL2dldFByb3RvdHlwZU9mLmpzIiwibm9kZV9tb2R1bGVzL0BiYWJlbC9ydW50aW1lL2hlbHBlcnMvaW5oZXJpdHMuanMiLCJub2RlX21vZHVsZXMvQGJhYmVsL3J1bnRpbWUvaGVscGVycy9pbnRlcm9wUmVxdWlyZURlZmF1bHQuanMiLCJub2RlX21vZHVsZXMvQGJhYmVsL3J1bnRpbWUvaGVscGVycy9pbnRlcm9wUmVxdWlyZVdpbGRjYXJkLmpzIiwibm9kZV9tb2R1bGVzL0BiYWJlbC9ydW50aW1lL2hlbHBlcnMvaXNOYXRpdmVGdW5jdGlvbi5qcyIsIm5vZGVfbW9kdWxlcy9AYmFiZWwvcnVudGltZS9oZWxwZXJzL2lzTmF0aXZlUmVmbGVjdENvbnN0cnVjdC5qcyIsIm5vZGVfbW9kdWxlcy9AYmFiZWwvcnVudGltZS9oZWxwZXJzL2l0ZXJhYmxlVG9BcnJheUxpbWl0LmpzIiwibm9kZV9tb2R1bGVzL0BiYWJlbC9ydW50aW1lL2hlbHBlcnMvbm9uSXRlcmFibGVSZXN0LmpzIiwibm9kZV9tb2R1bGVzL0BiYWJlbC9ydW50aW1lL2hlbHBlcnMvcG9zc2libGVDb25zdHJ1Y3RvclJldHVybi5qcyIsIm5vZGVfbW9kdWxlcy9AYmFiZWwvcnVudGltZS9oZWxwZXJzL3NldFByb3RvdHlwZU9mLmpzIiwibm9kZV9tb2R1bGVzL0BiYWJlbC9ydW50aW1lL2hlbHBlcnMvc2xpY2VkVG9BcnJheS5qcyIsIm5vZGVfbW9kdWxlcy9AYmFiZWwvcnVudGltZS9oZWxwZXJzL3R5cGVvZi5qcyIsIm5vZGVfbW9kdWxlcy9AYmFiZWwvcnVudGltZS9oZWxwZXJzL3Vuc3VwcG9ydGVkSXRlcmFibGVUb0FycmF5LmpzIiwibm9kZV9tb2R1bGVzL0BiYWJlbC9ydW50aW1lL2hlbHBlcnMvd3JhcE5hdGl2ZVN1cGVyLmpzIiwibm9kZV9tb2R1bGVzL0BiYWJlbC9ydW50aW1lL3JlZ2VuZXJhdG9yL2luZGV4LmpzIiwibm9kZV9tb2R1bGVzL3JlZ2VuZXJhdG9yLXJ1bnRpbWUvcnVudGltZS5qcyIsInNyYy9zZGsvYmFzZS9iYXNlNjQuanMiLCJzcmMvc2RrL2Jhc2UvY29kZWMuanMiLCJzcmMvc2RrL2Jhc2UvZXZlbnQuanMiLCJzcmMvc2RrL2Jhc2UvZXhwb3J0LmpzIiwic3JjL3Nkay9iYXNlL2xvZ2dlci5qcyIsInNyYy9zZGsvYmFzZS9tZWRpYWZvcm1hdC5qcyIsInNyYy9zZGsvYmFzZS9tZWRpYXN0cmVhbS1mYWN0b3J5LmpzIiwic3JjL3Nkay9iYXNlL3B1YmxpY2F0aW9uLmpzIiwic3JjL3Nkay9iYXNlL3NkcHV0aWxzLmpzIiwic3JjL3Nkay9iYXNlL3N0cmVhbS5qcyIsInNyYy9zZGsvYmFzZS90cmFuc3BvcnQuanMiLCJzcmMvc2RrL2Jhc2UvdXRpbHMuanMiLCJzcmMvc2RrL2NvbmZlcmVuY2UvY2hhbm5lbC5qcyIsInNyYy9zZGsvY29uZmVyZW5jZS9jbGllbnQuanMiLCJzcmMvc2RrL2NvbmZlcmVuY2UvZXJyb3IuanMiLCJzcmMvc2RrL2NvbmZlcmVuY2UvZXhwb3J0LmpzIiwic3JjL3Nkay9jb25mZXJlbmNlL2luZm8uanMiLCJzcmMvc2RrL2NvbmZlcmVuY2UvbWl4ZWRzdHJlYW0uanMiLCJzcmMvc2RrL2NvbmZlcmVuY2UvcGFydGljaXBhbnQuanMiLCJzcmMvc2RrL2NvbmZlcmVuY2UvcXVpY2Nvbm5lY3Rpb24uanMiLCJzcmMvc2RrL2NvbmZlcmVuY2Uvc2lnbmFsaW5nLmpzIiwic3JjL3Nkay9jb25mZXJlbmNlL3N0cmVhbXV0aWxzLmpzIiwic3JjL3Nkay9jb25mZXJlbmNlL3N1YnNjcmlwdGlvbi5qcyIsInNyYy9zZGsvZXhwb3J0LmpzIiwic3JjL3Nkay9wMnAvZXJyb3IuanMiLCJzcmMvc2RrL3AycC9leHBvcnQuanMiLCJzcmMvc2RrL3AycC9wMnBjbGllbnQuanMiLCJzcmMvc2RrL3AycC9wZWVyY29ubmVjdGlvbi1jaGFubmVsLmpzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0FDQUE7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUNWQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQ0pBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUNSQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUNwQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7O0FDTkE7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7O0FDckJBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7O0FDaEJBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7O0FDUEE7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQ2pCQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUNOQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUN0REE7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUNKQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQ2JBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQzNCQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQ0pBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQ1pBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQ1RBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQ1pBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7O0FDaEJBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUNYQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUMxQ0E7QUFDQTs7QUNEQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQzV1QkE7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBRUE7QUFDQTtBQUNBO0FBRUE7QUFFQTs7Ozs7OztBQUNPLElBQU0sTUFBTSxHQUFJLFlBQVc7QUFDaEMsTUFBTSxZQUFZLEdBQUcsQ0FBQyxDQUF0QjtBQUNBLE1BQUksU0FBSjtBQUNBLE1BQUksV0FBSjtBQUVBLE1BQUksQ0FBSjtBQUVBLE1BQU0sV0FBVyxHQUFHLENBQ2xCLEdBRGtCLEVBQ2IsR0FEYSxFQUNSLEdBRFEs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NUMsRUFBZ0QsV0FBaEQsR0FBOEQsSUFBOUQ7QUFDQSxtQkFBSyxhQUFMLENBQW1CLFdBQW5CO0FBQ0Q7QUFDRjtBQXBCOEM7QUFBQTtBQUFBO0FBQUE7QUFBQTtBQXFCaEQ7QUFDRjs7O3dCQWorQm9CO0FBQ25CLGFBQU8sS0FBSyxHQUFaO0FBQ0Q7OztFQTFDb0Msc0I7O2VBNGdDeEIsd0IiLCJmaWxlIjoiZ2VuZXJhdGVkLmpzIiwic291cmNlUm9vdCI6IiIsInNvdXJjZXNDb250ZW50IjpbIihmdW5jdGlvbigpe2Z1bmN0aW9uIHIoZSxuLHQpe2Z1bmN0aW9uIG8oaSxmKXtpZighbltpXSl7aWYoIWVbaV0pe3ZhciBjPVwiZnVuY3Rpb25cIj09dHlwZW9mIHJlcXVpcmUmJnJlcXVpcmU7aWYoIWYmJmMpcmV0dXJuIGMoaSwhMCk7aWYodSlyZXR1cm4gdShpLCEwKTt2YXIgYT1uZXcgRXJyb3IoXCJDYW5ub3QgZmluZCBtb2R1bGUgJ1wiK2krXCInXCIpO3Rocm93IGEuY29kZT1cIk1PRFVMRV9OT1RfRk9VTkRcIixhfXZhciBwPW5baV09e2V4cG9ydHM6e319O2VbaV1bMF0uY2FsbChwLmV4cG9ydHMsZnVuY3Rpb24ocil7dmFyIG49ZVtpXVsxXVtyXTtyZXR1cm4gbyhufHxyKX0scCxwLmV4cG9ydHMscixlLG4sdCl9cmV0dXJuIG5baV0uZXhwb3J0c31mb3IodmFyIHU9XCJmdW5jdGlvblwiPT10eXBlb2YgcmVxdWlyZSYmcmVxdWlyZSxpPTA7aTx0Lmxlbmd0aDtpKyspbyh0W2ldKTtyZXR1cm4gb31yZXR1cm4gcn0pKCkiLCJmdW5jdGlvbiBfYXJyYXlMaWtlVG9BcnJheShhcnIsIGxlbikge1xuICBpZiAobGVuID09IG51bGwgfHwgbGVuID4gYXJyLmxlbmd0aCkgbGVuID0gYXJyLmxlbmd0aDtcblxuICBmb3IgKHZhciBpID0gMCwgYXJyMiA9IG5ldyBBcnJheShsZW4pOyBpIDwgbGVuOyBpKyspIHtcbiAgICBhcnIyW2ldID0gYXJyW2ldO1xuICB9XG5cbiAgcmV0dXJuIGFycjI7XG59XG5cbm1vZHVsZS5leHBvcnRzID0gX2FycmF5TGlrZVRvQXJyYXk7IiwiZnVuY3Rpb24gX2FycmF5V2l0aEhvbGVzKGFycikge1xuICBpZiAoQXJyYXkuaXNBcnJheShhcnIpKSByZXR1cm4gYXJyO1xufVxuXG5tb2R1bGUuZXhwb3J0cyA9IF9hcnJheVdpdGhIb2xlczsiLCJmdW5jdGlvbiBfYXNzZXJ0VGhpc0luaXRpYWxpemVkKHNlbGYpIHtcbiAgaWYgKHNlbGYgPT09IHZvaWQgMCkge1xuICAgIHRocm93IG5ldyBSZWZlcmVuY2VFcnJvcihcInRoaXMgaGFzbid0IGJlZW4gaW5pdGlhbGlzZWQgLSBzdXBlcigpIGhhc24ndCBiZWVuIGNhbGxlZFwiKTtcbiAgfVxuXG4gIHJldHVybiBzZWxmO1xufVxuXG5tb2R1bGUuZXhwb3J0cyA9IF9hc3NlcnRUaGlzSW5pdGlhbGl6ZWQ7IiwiZnVuY3Rpb24gYXN5bmNHZW5lcmF0b3JTdGVwKGdlbiwgcmVzb2x2ZSwgcmVqZWN0LCBfbmV4dCwgX3Rocm93LCBrZXksIGFyZykge1xuICB0cnkge1xuICAgIHZhciBpbmZvID0gZ2VuW2tleV0oYXJnKTtcbiAgICB2YXIgdmFsdWUgPSBpbmZvLnZhbHVlO1xuICB9IGNhdGNoIChlcnJvcikge1xuICAgIHJlamVjdChlcnJvcik7XG4gICAgcmV0dXJuO1xuICB9XG5cbiAgaWYgKGluZm8uZG9uZSkge1xuICAgIHJlc29sdmUodmFsdWUpO1xuICB9IGVsc2Uge1xuICAgIFByb21pc2UucmVzb2x2ZSh2YWx1ZSkudGhlbihfbmV4dCwgX3Rocm93KTtcbiAgfVxufVxuXG5mdW5jdGlvbiBfYXN5bmNUb0dlbmVyYXRvcihmbikge1xuICByZXR1cm4gZnVuY3Rpb24gKCkge1xuICAgIHZhciBzZWxmID0gdGhpcyxcbiAgICAgICAgYXJncyA9IGFyZ3VtZW50cztcbiAgICByZXR1cm4gbmV3IFByb21pc2UoZnVuY3Rpb24gKHJlc29sdmUsIHJlamVjdCkge1xuICAgICAgdmFyIGdlbiA9IGZuLmFwcGx5KHNlbGYsIGFyZ3MpO1xuXG4gICAgICBmdW5jdGlvbiBfbmV4dCh2YWx1ZSkge1xuICAgICAgICBhc3luY0dlbmVyYXRvclN0ZXAoZ2VuLCByZXNvbHZlLCByZWplY3QsIF9uZXh0LCBfdGhyb3csIFwibmV4dFwiLCB2YWx1ZSk7XG4gICAgICB9XG5cbiAgICAgIGZ1bmN0aW9uIF90aHJvdyhlcnIpIHtcbiAgICAgICAgYXN5bmNHZW5lcmF0b3JTdGVwKGdlbiwgcmVzb2x2ZSwgcmVqZWN0LCBfbmV4dCwgX3Rocm93LCBcInRocm93XCIsIGVycik7XG4gICAgICB9XG5cbiAgICAgIF9uZXh0KHVuZGVmaW5lZCk7XG4gICAgfSk7XG4gIH07XG59XG5cbm1vZHVsZS5leHBvcnRzID0gX2FzeW5jVG9HZW5lcmF0b3I7IiwiZnVuY3Rpb24gX2NsYXNzQ2FsbENoZWNrKGluc3RhbmNlLCBDb25zdHJ1Y3Rvcikge1xuICBpZiAoIShpbnN0YW5jZSBpbnN0YW5jZW9mIENvbnN0cnVjdG9yKSkge1xuICAgIHRocm93IG5ldyBUeXBlRXJyb3IoXCJDYW5ub3QgY2FsbCBhIGNsYXNzIGFzIGEgZnVuY3Rpb25cIik7XG4gIH1cbn1cblxubW9kdWxlLmV4cG9ydHMgPSBfY2xhc3NDYWxsQ2hlY2s7IiwidmFyIHNldFByb3RvdHlwZU9mID0gcmVxdWlyZShcIi4vc2V0UHJvdG90eXBlT2ZcIik7XG5cbnZhciBpc05hdGl2ZVJlZmxlY3RDb25zdHJ1Y3QgPSByZXF1aXJlKFwiLi9pc05hdGl2ZVJlZmxlY3RDb25zdHJ1Y3RcIik7XG5cbmZ1bmN0aW9uIF9jb25zdHJ1Y3QoUGFyZW50LCBhcmdzLCBDbGFzcykge1xuICBpZiAoaXNOYXRpdmVSZWZsZWN0Q29uc3RydWN0KCkpIHtcbiAgICBtb2R1bGUuZXhwb3J0cyA9IF9jb25zdHJ1Y3QgPSBSZWZsZWN0LmNvbnN0cnVjdDtcbiAgfSBlbHNlIHtcbiAgICBtb2R1bGUuZXhwb3J0cyA9IF9jb25zdHJ1Y3QgPSBmdW5jdGlvbiBfY29uc3RydWN0KFBhcmVudCwgYXJncywgQ2xhc3MpIHtcbiAgICAgIHZhciBhID0gW251bGxdO1xuICAgICAgYS5wdXNoLmFwcGx5KGEsIGFyZ3MpO1xuICAgICAgdmFyIENvbnN0cnVjdG9yID0gRnVuY3Rpb24uYmluZC5hcHBseShQYXJlbnQsIGEpO1xuICAgICAgdmFyIGluc3RhbmNlID0gbmV3IENvbnN0cnVjdG9yKCk7XG4gICAgICBpZiAoQ2xhc3MpIHNldFByb3RvdHlwZU9mKGluc3RhbmNlLCBDbGFzcy5wcm90b3R5cGUpO1xuICAgICAgcmV0dXJuIGluc3RhbmNlO1xuICAgIH07XG4gIH1cblxuICByZXR1cm4gX2NvbnN0cnVjdC5hcHBseShudWxsLCBhcmd1bWVudHMpO1xufVxuXG5tb2R1bGUuZXhwb3J0cyA9IF9jb25zdHJ1Y3Q7IiwiZnVuY3Rpb24gX2RlZmluZVByb3BlcnRpZXModGFyZ2V0LCBwcm9wcykge1xuICBmb3IgKHZhciBpID0gMDsgaSA8IHByb3BzLmxlbmd0aDsgaSsrKSB7XG4gICAgdmFyIGRlc2NyaXB0b3IgPSBwcm9wc1tpXTtcbiAgICBkZXNjcmlwdG9yLmVudW1lcmFibGUgPSBkZXNjcmlwdG9yLmVudW1lcmFibGUgfHwgZmFsc2U7XG4gICAgZGVzY3JpcHRvci5jb25maWd1cmFibGUgPSB0cnVlO1xuICAgIGlmIChcInZhbHVlXCIgaW4gZGVzY3JpcHRvcikgZGVzY3JpcHRvci53cml0YWJsZSA9IHRydWU7XG4gICAgT2JqZWN0LmRlZmluZVByb3BlcnR5KHRhcmdldCwgZGVzY3JpcHRvci5rZXksIGRlc2NyaXB0b3IpO1xuICB9XG59XG5cbmZ1bmN0aW9uIF9jcmVhdGVDbGFzcyhDb25zdHJ1Y3RvciwgcHJvdG9Qcm9wcywgc3RhdGljUHJvcHMpIHtcbiAgaWYgKHByb3RvUHJvcHMpIF9kZWZpbmVQcm9wZXJ0aWVzKENvbnN0cnVjdG9yLnByb3RvdHlwZSwgcHJvdG9Qcm9wcyk7XG4gIGlmIChzdGF0aWNQcm9wcykgX2RlZmluZVByb3BlcnRpZXMoQ29uc3RydWN0b3IsIHN0YXRpY1Byb3BzKTtcbiAgcmV0dXJuIENvbnN0cnVjdG9yO1xufVxuXG5tb2R1bGUuZXhwb3J0cyA9IF9jcmVhdGVDbGFzczsiLCJmdW5jdGlvbiBfZ2V0UHJvdG90eXBlT2Yobykge1xuICBtb2R1bGUuZXhwb3J0cyA9IF9nZXRQcm90b3R5cGVPZiA9IE9iamVjdC5zZXRQcm90b3R5cGVPZiA/IE9iamVjdC5nZXRQcm90b3R5cGVPZiA6IGZ1bmN0aW9uIF9nZXRQcm90b3R5cGVPZihvKSB7XG4gICAgcmV0dXJuIG8uX19wcm90b19fIHx8IE9iamVjdC5nZXRQcm90b3R5cGVPZihvKTtcbiAgfTtcbiAgcmV0dXJuIF9nZXRQcm90b3R5cGVPZihvKTtcbn1cblxubW9kdWxlLmV4cG9ydHMgPSBfZ2V0UHJvdG90eXBlT2Y7IiwidmFyIHNldFByb3RvdHlwZU9mID0gcmVxdWlyZShcIi4vc2V0UHJvdG90eXBlT2ZcIik7XG5cbmZ1bmN0aW9uIF9pbmhlcml0cyhzdWJDbGFzcywgc3VwZXJDbGFzcykge1xuICBpZiAodHlwZW9mIHN1cGVyQ2xhc3MgIT09IFwiZnVuY3Rpb25cIiAmJiBzdXBlckNsYXNzICE9PSBudWxsKSB7XG4gICAgdGhyb3cgbmV3IFR5cGVFcnJvcihcIlN1cGVyIGV4cHJlc3Npb24gbXVzdCBlaXRoZXIgYmUgbnVsbCBvciBhIGZ1bmN0aW9uXCIpO1xuICB9XG5cbiAgc3ViQ2xhc3MucHJvdG90eXBlID0gT2JqZWN0LmNyZWF0ZShzdXBlckNsYXNzICYmIHN1cGVyQ2xhc3MucHJvdG90eXBlLCB7XG4gICAgY29uc3RydWN0b3I6IHtcbiAgICAgIHZhbHVlOiBzdWJDbGFzcyxcbiAgICAgIHdyaXRhYmxlOiB0cnVlLFxuICAgICAgY29uZmlndXJhYmxlOiB0cnVlXG4gICAgfVxuICB9KTtcbiAgaWYgKHN1cGVyQ2xhc3MpIHNldFByb3RvdHlwZU9mKHN1YkNsYXNzLCBzdXBlckNsYXNzKTtcbn1cblxubW9kdWxlLmV4cG9ydHMgPSBfaW5oZXJpdHM7IiwiZnVuY3Rpb24gX2ludGVyb3BSZXF1aXJlRGVmYXVsdChvYmopIHtcbiAgcmV0dXJuIG9iaiAmJiBvYmouX19lc01vZHVsZSA/IG9iaiA6IHtcbiAgICBcImRlZmF1bHRcIjogb2JqXG4gIH07XG59XG5cbm1vZHVsZS5leHBvcnRzID0gX2ludGVyb3BSZXF1aXJlRGVmYXVsdDsiLCJ2YXIgX3R5cGVvZiA9IHJlcXVpcmUoXCJAYmFiZWwvcnVudGltZS9oZWxwZXJzL3R5cGVvZlwiKTtcblxuZnVuY3Rpb24gX2dldFJlcXVpcmVXaWxkY2FyZENhY2hlKCkge1xuICBpZiAodHlwZW9mIFdlYWtNYXAgIT09IFwiZnVuY3Rpb25cIikgcmV0dXJuIG51bGw7XG4gIHZhciBjYWNoZSA9IG5ldyBXZWFrTWFwKCk7XG5cbiAgX2dldFJlcXVpcmVXaWxkY2FyZENhY2hlID0gZnVuY3Rpb24gX2dldFJlcXVpcmVXaWxkY2FyZENhY2hlKCkge1xuICAgIHJldHVybiBjYWNoZTtcbiAgfTtcblxuICByZXR1cm4gY2FjaGU7XG59XG5cbmZ1bmN0aW9uIF9pbnRlcm9wUmVxdWlyZVdpbGRjYXJkKG9iaikge1xuICBpZiAob2JqICYmIG9iai5fX2VzTW9kdWxlKSB7XG4gICAgcmV0dXJuIG9iajtcbiAgfVxuXG4gIGlmIChvYmogPT09IG51bGwgfHwgX3R5cGVvZihvYmopICE9PSBcIm9iamVjdFwiICYmIHR5cGVvZiBvYmogIT09IFwiZnVuY3Rpb25cIikge1xuICAgIHJldHVybiB7XG4gICAgICBcImRlZmF1bHRcIjogb2JqXG4gICAgfTtcbiAgfVxuXG4gIHZhciBjYWNoZSA9IF9nZXRSZXF1aXJlV2lsZGNhcmRDYWNoZSgpO1xuXG4gIGlmIChjYWNoZSAmJiBjYWNoZS5oYXMob2JqKSkge1xuICAgIHJldHVybiBjYWNoZS5nZXQob2JqKTtcbiAgfVxuXG4gIHZhciBuZXdPYmogPSB7fTtcbiAgdmFyIGhhc1Byb3BlcnR5RGVzY3JpcHRvciA9IE9iamVjdC5kZWZpbmVQcm9wZXJ0eSAmJiBPYmplY3QuZ2V0T3duUHJvcGVydHlEZXNjcmlwdG9yO1xuXG4gIGZvciAodmFyIGtleSBpbiBvYmopIHtcbiAgICBpZiAoT2JqZWN0LnByb3RvdHlwZS5oYXNPd25Qcm9wZXJ0eS5jYWxsKG9iaiwga2V5KSkge1xuICAgICAgdmFyIGRlc2MgPSBoYXNQcm9wZXJ0eURlc2NyaXB0b3IgPyBPYmplY3QuZ2V0T3duUHJvcGVydHlEZXNjcmlwdG9yKG9iaiwga2V5KSA6IG51bGw7XG5cbiAgICAgIGlmIChkZXNjICYmIChkZXNjLmdldCB8fCBkZXNjLnNldCkpIHtcbiAgICAgICAgT2JqZWN0LmRlZmluZVByb3BlcnR5KG5ld09iaiwga2V5LCBkZXNjKTtcbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIG5ld09ialtrZXldID0gb2JqW2tleV07XG4gICAgICB9XG4gICAgfVxuICB9XG5cbiAgbmV3T2JqW1wiZGVmYXVsdFwiXSA9IG9iajtcblxuICBpZiAoY2FjaGUpIHtcbiAgICBjYWNoZS5zZXQob2JqLCBuZXdPYmopO1xuICB9XG5cbiAgcmV0dXJuIG5ld09iajtcbn1cblxubW9kdWxlLmV4cG9ydHMgPSBfaW50ZXJvcFJlcXVpcmVXaWxkY2FyZDsiLCJmdW5jdGlvbiBfaXNOYXRpdmVGdW5jdGlvbihmbikge1xuICByZXR1cm4gRnVuY3Rpb24udG9TdHJpbmcuY2FsbChmbikuaW5kZXhPZihcIltuYXRpdmUgY29kZV1cIikgIT09IC0xO1xufVxuXG5tb2R1bGUuZXhwb3J0cyA9IF9pc05hdGl2ZUZ1bmN0aW9uOyIsImZ1bmN0aW9uIF9pc05hdGl2ZVJlZmxlY3RDb25zdHJ1Y3QoKSB7XG4gIGlmICh0eXBlb2YgUmVmbGVjdCA9PT0gXCJ1bmRlZmluZWRcIiB8fCAhUmVmbGVjdC5jb25zdHJ1Y3QpIHJldHVybiBmYWxzZTtcbiAgaWYgKFJlZmxlY3QuY29uc3RydWN0LnNoYW0pIHJldHVybiBmYWxzZTtcbiAgaWYgKHR5cGVvZiBQcm94eSA9PT0gXCJmdW5jdGlvblwiKSByZXR1cm4gdHJ1ZTtcblxuICB0cnkge1xuICAgIERhdGUucHJvdG90eXBlLnRvU3RyaW5nLmNhbGwoUmVmbGVjdC5jb25zdHJ1Y3QoRGF0ZSwgW10sIGZ1bmN0aW9uICgpIHt9KSk7XG4gICAgcmV0dXJuIHRydWU7XG4gIH0gY2F0Y2ggKGUpIHtcbiAgICByZXR1cm4gZmFsc2U7XG4gIH1cbn1cblxubW9kdWxlLmV4cG9ydHMgPSBfaXNOYXRpdmVSZWZsZWN0Q29uc3RydWN0OyIsImZ1bmN0aW9uIF9pdGVyYWJsZVRvQXJyYXlMaW1pdChhcnIsIGkpIHtcbiAgaWYgKHR5cGVvZiBTeW1ib2wgPT09IFwidW5kZWZpbmVkXCIgfHwgIShTeW1ib2wuaXRlcmF0b3IgaW4gT2JqZWN0KGFycikpKSByZXR1cm47XG4gIHZhciBfYXJyID0gW107XG4gIHZhciBfbiA9IHRydWU7XG4gIHZhciBfZCA9IGZhbHNlO1xuICB2YXIgX2UgPSB1bmRlZmluZWQ7XG5cbiAgdHJ5IHtcbiAgICBmb3IgKHZhciBfaSA9IGFycltTeW1ib2wuaXRlcmF0b3JdKCksIF9zOyAhKF9uID0gKF9zID0gX2kubmV4dCgpKS5kb25lKTsgX24gPSB0cnVlKSB7XG4gICAgICBfYXJyLnB1c2goX3MudmFsdWUpO1xuXG4gICAgICBpZiAoaSAmJiBfYXJyLmxlbmd0aCA9PT0gaSkgYnJlYWs7XG4gICAgfVxuICB9IGNhdGNoIChlcnIpIHtcbiAgICBfZCA9IHRydWU7XG4gICAgX2UgPSBlcnI7XG4gIH0gZmluYWxseSB7XG4gICAgdHJ5IHtcbiAgICAgIGlmICghX24gJiYgX2lbXCJyZXR1cm5cIl0gIT0gbnVsbCkgX2lbXCJyZXR1cm5cIl0oKTtcbiAgICB9IGZpbmFsbHkge1xuICAgICAgaWYgKF9kKSB0aHJvdyBfZTtcbiAgICB9XG4gIH1cblxuICByZXR1cm4gX2Fycjtcbn1cblxubW9kdWxlLmV4cG9ydHMgPSBfaXRlcmFibGVUb0FycmF5TGltaXQ7IiwiZnVuY3Rpb24gX25vbkl0ZXJhYmxlUmVzdCgpIHtcbiAgdGhyb3cgbmV3IFR5cGVFcnJvcihcIkludmFsaWQgYXR0ZW1wdCB0byBkZXN0cnVjdHVyZSBub24taXRlcmFibGUgaW5zdGFuY2UuXFxuSW4gb3JkZXIgdG8gYmUgaXRlcmFibGUsIG5vbi1hcnJheSBvYmplY3RzIG11c3QgaGF2ZSBhIFtTeW1ib2wuaXRlcmF0b3JdKCkgbWV0aG9kLlwiKTtcbn1cblxubW9kdWxlLmV4cG9ydHMgPSBfbm9uSXRlcmFibGVSZXN0OyIsInZhciBfdHlwZW9mID0gcmVxdWlyZShcIkBiYWJlbC9ydW50aW1lL2hlbHBlcnMvdHlwZW9mXCIpO1xuXG52YXIgYXNzZXJ0VGhpc0luaXRpYWxpemVkID0gcmVxdWlyZShcIi4vYXNzZXJ0VGhpc0luaXRpYWxpemVkXCIpO1xuXG5mdW5jdGlvbiBfcG9zc2libGVDb25zdHJ1Y3RvclJldHVybihzZWxmLCBjYWxsKSB7XG4gIGlmIChjYWxsICYmIChfdHlwZW9mKGNhbGwpID09PSBcIm9iamVjdFwiIHx8IHR5cGVvZiBjYWxsID09PSBcImZ1bmN0aW9uXCIpKSB7XG4gICAgcmV0dXJuIGNhbGw7XG4gIH1cblxuICByZXR1cm4gYXNzZXJ0VGhpc0luaXRpYWxpemVkKHNlbGYpO1xufVxuXG5tb2R1bGUuZXhwb3J0cyA9IF9wb3NzaWJsZUNvbnN0cnVjdG9yUmV0dXJuOyIsImZ1bmN0aW9uIF9zZXRQcm90b3R5cGVPZihvLCBwKSB7XG4gIG1vZHVsZS5leHBvcnRzID0gX3NldFByb3RvdHlwZU9mID0gT2JqZWN0LnNldFByb3RvdHlwZU9mIHx8IGZ1bmN0aW9uIF9zZXRQcm90b3R5cGVPZihvLCBwKSB7XG4gICAgby5fX3Byb3RvX18gPSBwO1xuICAgIHJldHVybiBvO1xuICB9O1xuXG4gIHJldHVybiBfc2V0UHJvdG90eXBlT2YobywgcCk7XG59XG5cbm1vZHVsZS5leHBvcnRzID0gX3NldFByb3RvdHlwZU9mOyIsInZhciBhcnJheVdpdGhIb2xlcyA9IHJlcXVpcmUoXCIuL2FycmF5V2l0aEhvbGVzXCIpO1xuXG52YXIgaXRlcmFibGVUb0FycmF5TGltaXQgPSByZXF1aXJlKFwiLi9pdGVyYWJsZVRvQXJyYXlMaW1pdFwiKTtcblxudmFyIHVuc3VwcG9ydGVkSXRlcmFibGVUb0FycmF5ID0gcmVxdWlyZShcIi4vdW5zdXBwb3J0ZWRJdGVyYWJsZVRvQXJyYXlcIik7XG5cbnZhciBub25JdGVyYWJsZVJlc3QgPSByZXF1aXJlKFwiLi9ub25JdGVyYWJsZVJlc3RcIik7XG5cbmZ1bmN0aW9uIF9zbGljZWRUb0FycmF5KGFyciwgaSkge1xuICByZXR1cm4gYXJyYXlXaXRoSG9sZXMoYXJyKSB8fCBpdGVyYWJsZVRvQXJyYXlMaW1pdChhcnIsIGkpIHx8IHVuc3VwcG9ydGVkSXRlcmFibGVUb0FycmF5KGFyciwgaSkgfHwgbm9uSXRlcmFibGVSZXN0KCk7XG59XG5cbm1vZHVsZS5leHBvcnRzID0gX3NsaWNlZFRvQXJyYXk7IiwiZnVuY3Rpb24gX3R5cGVvZihvYmopIHtcbiAgXCJAYmFiZWwvaGVscGVycyAtIHR5cGVvZlwiO1xuXG4gIGlmICh0eXBlb2YgU3ltYm9sID09PSBcImZ1bmN0aW9uXCIgJiYgdHlwZW9mIFN5bWJvbC5pdGVyYXRvciA9PT0gXCJzeW1ib2xcIikge1xuICAgIG1vZHVsZS5leHBvcnRzID0gX3R5cGVvZiA9IGZ1bmN0aW9uIF90eXBlb2Yob2JqKSB7XG4gICAgICByZXR1cm4gdHlwZW9mIG9iajtcbiAgICB9O1xuICB9IGVsc2Uge1xuICAgIG1vZHVsZS5leHBvcnRzID0gX3R5cGVvZiA9IGZ1bmN0aW9uIF90eXBlb2Yob2JqKSB7XG4gICAgICByZXR1cm4gb2JqICYmIHR5cGVvZiBTeW1ib2wgPT09IFwiZnVuY3Rpb25cIiAmJiBvYmouY29uc3RydWN0b3IgPT09IFN5bWJvbCAmJiBvYmogIT09IFN5bWJvbC5wcm90b3R5cGUgPyBcInN5bWJvbFwiIDogdHlwZW9mIG9iajtcbiAgICB9O1xuICB9XG5cbiAgcmV0dXJuIF90eXBlb2Yob2JqKTtcbn1cblxubW9kdWxlLmV4cG9ydHMgPSBfdHlwZW9mOyIsInZhciBhcnJheUxpa2VUb0FycmF5ID0gcmVxdWlyZShcIi4vYXJyYXlMaWtlVG9BcnJheVwiKTtcblxuZnVuY3Rpb24gX3Vuc3VwcG9ydGVkSXRlcmFibGVUb0FycmF5KG8sIG1pbkxlbikge1xuICBpZiAoIW8pIHJldHVybjtcbiAgaWYgKHR5cGVvZiBvID09PSBcInN0cmluZ1wiKSByZXR1cm4gYXJyYXlMaWtlVG9BcnJheShvLCBtaW5MZW4pO1xuICB2YXIgbiA9IE9iamVjdC5wcm90b3R5cGUudG9TdHJpbmcuY2FsbChvKS5zbGljZSg4LCAtMSk7XG4gIGlmIChuID09PSBcIk9iamVjdFwiICYmIG8uY29uc3RydWN0b3IpIG4gPSBvLmNvbnN0cnVjdG9yLm5hbWU7XG4gIGlmIChuID09PSBcIk1hcFwiIHx8IG4gPT09IFwiU2V0XCIpIHJldHVybiBBcnJheS5mcm9tKG8pO1xuICBpZiAobiA9PT0gXCJBcmd1bWVudHNcIiB8fCAvXig/OlVpfEkpbnQoPzo4fDE2fDMyKSg/OkNsYW1wZWQpP0FycmF5JC8udGVzdChuKSkgcmV0dXJuIGFycmF5TGlrZVRvQXJyYXkobywgbWluTGVuKTtcbn1cblxubW9kdWxlLmV4cG9ydHMgPSBfdW5zdXBwb3J0ZWRJdGVyYWJsZVRvQXJyYXk7IiwidmFyIGdldFByb3RvdHlwZU9mID0gcmVxdWlyZShcIi4vZ2V0UHJvdG90eXBlT2ZcIik7XG5cbnZhciBzZXRQcm90b3R5cGVPZiA9IHJlcXVpcmUoXCIuL3NldFByb3RvdHlwZU9mXCIpO1xuXG52YXIgaXNOYXRpdmVGdW5jdGlvbiA9IHJlcXVpcmUoXCIuL2lzTmF0aXZlRnVuY3Rpb25cIik7XG5cbnZhciBjb25zdHJ1Y3QgPSByZXF1aXJlKFwiLi9jb25zdHJ1Y3RcIik7XG5cbmZ1bmN0aW9uIF93cmFwTmF0aXZlU3VwZXIoQ2xhc3MpIHtcbiAgdmFyIF9jYWNoZSA9IHR5cGVvZiBNYXAgPT09IFwiZnVuY3Rpb25cIiA/IG5ldyBNYXAoKSA6IHVuZGVmaW5lZDtcblxuICBtb2R1bGUuZXhwb3J0cyA9IF93cmFwTmF0aXZlU3VwZXIgPSBmdW5jdGlvbiBfd3JhcE5hdGl2ZVN1cGVyKENsYXNzKSB7XG4gICAgaWYgKENsYXNzID09PSBudWxsIHx8ICFpc05hdGl2ZUZ1bmN0aW9uKENsYXNzKSkgcmV0dXJuIENsYXNzO1xuXG4gICAgaWYgKHR5cGVvZiBDbGFzcyAhPT0gXCJmdW5jdGlvblwiKSB7XG4gICAgICB0aHJvdyBuZXcgVHlwZUVycm9yKFwiU3VwZXIgZXhwcmVzc2lvbiBtdXN0IGVpdGhlciBiZSBudWxsIG9yIGEgZnVuY3Rpb25cIik7XG4gICAgfVxuXG4gICAgaWYgKHR5cGVvZiBfY2FjaGUgIT09IFwidW5kZWZpbmVkXCIpIHtcbiAgICAgIGlmIChfY2FjaGUuaGFzKENsYXNzKSkgcmV0dXJuIF9jYWNoZS5nZXQoQ2xhc3MpO1xuXG4gICAgICBfY2FjaGUuc2V0KENsYXNzLCBXcmFwcGVyKTtcbiAgICB9XG5cbiAgICBmdW5jdGlvbiBXcmFwcGVyKCkge1xuICAgICAgcmV0dXJuIGNvbnN0cnVjdChDbGFzcywgYXJndW1lbnRzLCBnZXRQcm90b3R5cGVPZih0aGlzKS5jb25zdHJ1Y3Rvcik7XG4gICAgfVxuXG4gICAgV3JhcHBlci5wcm90b3R5cGUgPSBPYmplY3QuY3JlYXRlKENsYXNzLnByb3RvdHlwZSwge1xuICAgICAgY29uc3RydWN0b3I6IHtcbiAgICAgICAgdmFsdWU6IFdyYXBwZXIsXG4gICAgICAgIGVudW1lcmFibGU6IGZhbHNlLFxuICAgICAgICB3cml0YWJsZTogdHJ1ZSxcbiAgICAgICAgY29uZmlndXJhYmxlOiB0cnVlXG4gICAgICB9XG4gICAgfSk7XG4gICAgcmV0dXJuIHNldFByb3RvdHlwZU9mKFdyYXBwZXIsIENsYXNzKTtcbiAgfTtcblxuICByZXR1cm4gX3dyYXBOYXRpdmVTdXBlcihDbGFzcyk7XG59XG5cbm1vZHVsZS5leHBvcnRzID0gX3dyYXBOYXRpdmVTdXBlcjsiLCJtb2R1bGUuZXhwb3J0cyA9IHJlcXVpcmUoXCJyZWdlbmVyYXRvci1ydW50aW1lXCIpO1xuIiwiLyoqXG4gKiBDb3B5cmlnaHQgKGMpIDIwMTQtcHJlc2VudCwgRmFjZWJvb2ssIEluYy5cbiAqXG4gKiBUaGlzIHNvdXJjZSBjb2RlIGlzIGxpY2Vuc2VkIHVuZGVyIHRoZSBNSVQgbGljZW5zZSBmb3VuZCBpbiB0aGVcbiAqIExJQ0VOU0UgZmlsZSBpbiB0aGUgcm9vdCBkaXJlY3Rvcnkgb2YgdGhpcyBzb3VyY2UgdHJlZS5cbiAqL1xuXG52YXIgcnVudGltZSA9IChmdW5jdGlvbiAoZXhwb3J0cykge1xuICBcInVzZSBzdHJpY3RcIjtcblxuICB2YXIgT3AgPSBPYmplY3QucHJvdG90eXBlO1xuICB2YXIgaGFzT3duID0gT3AuaGFzT3duUHJvcGVydHk7XG4gIHZhciB1bmRlZmluZWQ7IC8vIE1vcmUgY29tcHJlc3NpYmxlIHRoYW4gdm9pZCAwLlxuICB2YXIgJFN5bWJvbCA9IHR5cGVvZiBTeW1ib2wgPT09IFwiZnVuY3Rpb25cIiA/IFN5bWJvbCA6IHt9O1xuICB2YXIgaXRlcmF0b3JTeW1ib2wgPSAkU3ltYm9sLml0ZXJhdG9yIHx8IFwiQEBpdGVyYXRvclwiO1xuICB2YXIgYXN5bmNJdGVyYXRvclN5bWJvbCA9ICRTeW1ib2wuYXN5bmNJdGVyYXRvciB8fCBcIkBAYXN5bmNJdGVyYXRvclwiO1xuICB2YXIgdG9TdHJpbmdUYWdTeW1ib2wgPSAkU3ltYm9sLnRvU3RyaW5nVGFnIHx8IFwiQEB0b1N0cmluZ1RhZ1wiO1xuXG4gIGZ1bmN0aW9uIGRlZmluZShvYmosIGtleSwgdmFsdWUpIHtcbiAgICBPYmplY3QuZGVmaW5lUHJvcGVydHkob2JqLCBrZXksIHtcbiAgICAgIHZhbHVlOiB2YWx1ZSxcbiAgICAgIGVudW1lcmFibGU6IHRydWUsXG4gICAgICBjb25maWd1cmFibGU6IHRydWUsXG4gICAgICB3cml0YWJsZTogdHJ1ZVxuICAgIH0pO1xuICAgIHJldHVybiBvYmpba2V5XTtcbiAgfVxuICB0cnkge1xuICAgIC8vIElFIDggaGFzIGEgYnJva2VuIE9iamVjdC5kZWZpbmVQcm9wZXJ0eSB0aGF0IG9ubHkgd29ya3Mgb24gRE9NIG9iamVjdHMuXG4gICAgZGVmaW5lKHt9LCBcIlwiKTtcbiAgfSBjYXRjaCAoZXJyKSB7XG4gICAgZGVmaW5lID0gZnVuY3Rpb24ob2JqLCBrZXksIHZhbHVlKSB7XG4gICAgICByZXR1cm4gb2JqW2tleV0gPSB2YWx1ZTtcbiAgICB9O1xuICB9XG5cbiAgZnVuY3Rpb24gd3JhcChpbm5lckZuLCBvdXRlckZuLCBzZWxmLCB0cnlMb2NzTGlzdCkge1xuICAgIC8vIElmIG91dGVyRm4gcHJvdmlkZWQgYW5kIG91dGVyRm4ucHJvdG90eXBlIGlzIGEgR2VuZXJhdG9yLCB0aGVuIG91dGVyRm4ucHJvdG90eXBlIGluc3RhbmNlb2YgR2VuZXJhdG9yLlxuICAgIHZhciBwcm90b0dlbmVyYXRvciA9IG91dGVyRm4gJiYgb3V0ZXJGbi5wcm90b3R5cGUgaW5zdGFuY2VvZiBHZW5lcmF0b3IgPyBvdXRlckZuIDogR2VuZXJhdG9yO1xuICAgIHZhciBnZW5lcmF0b3IgPSBPYmplY3QuY3JlYXRlKHByb3RvR2VuZXJhdG9yLnByb3RvdHlwZSk7XG4gICAgdmFyIGNvbnRleHQgPSBuZXcgQ29udGV4dCh0cnlMb2NzTGlzdCB8fCBbXSk7XG5cbiAgICAvLyBUaGUgLl9pbnZva2UgbWV0aG9kIHVuaWZpZXMgdGhlIGltcGxlbWVudGF0aW9ucyBvZiB0aGUgLm5leHQsXG4gICAgLy8gLnRocm93LCBhbmQgLnJldHVybiBtZXRob2RzLlxuICAgIGdlbmVyYXRvci5faW52b2tlID0gbWFrZUludm9rZU1ldGhvZChpbm5lckZuLCBzZWxmLCBjb250ZXh0KTtcblxuICAgIHJldHVybiBnZW5lcmF0b3I7XG4gIH1cbiAgZXhwb3J0cy53cmFwID0gd3JhcDtcblxuICAvLyBUcnkvY2F0Y2ggaGVscGVyIHRvIG1pbmltaXplIGRlb3B0aW1pemF0aW9ucy4gUmV0dXJucyBhIGNvbXBsZXRpb25cbiAgLy8gcmVjb3JkIGxpa2UgY29udGV4dC50cnlFbnRyaWVzW2ldLmNvbXBsZXRpb24uIFRoaXMgaW50ZXJmYWNlIGNvdWxkXG4gIC8vIGhhdmUgYmVlbiAoYW5kIHdhcyBwcmV2aW91c2x5KSBkZXNpZ25lZCB0byB0YWtlIGEgY2xvc3VyZSB0byBiZVxuICAvLyBpbnZva2VkIHdpdGhvdXQgYXJndW1lbnRzLCBidXQgaW4gYWxsIHRoZSBjYXNlcyB3ZSBjYXJlIGFib3V0IHdlXG4gIC8vIGFscmVhZHkgaGF2ZSBhbiBleGlzdGluZyBtZXRob2Qgd2Ugd2FudCB0byBjYWxsLCBzbyB0aGVyZSdzIG5vIG5lZWRcbiAgLy8gdG8gY3JlYXRlIGEgbmV3IGZ1bmN0aW9uIG9iamVjdC4gV2UgY2FuIGV2ZW4gZ2V0IGF3YXkgd2l0aCBhc3N1bWluZ1xuICAvLyB0aGUgbWV0aG9kIHRha2VzIGV4YWN0bHkgb25lIGFyZ3VtZW50LCBzaW5jZSB0aGF0IGhhcHBlbnMgdG8gYmUgdHJ1ZVxuICAvLyBpbiBldmVyeSBjYXNlLCBzbyB3ZSBkb24ndCBoYXZlIHRvIHRvdWNoIHRoZSBhcmd1bWVudHMgb2JqZWN0LiBUaGVcbiAgLy8gb25seSBhZGRpdGlvbmFsIGFsbG9jYXRpb24gcmVxdWlyZWQgaXMgdGhlIGNvbXBsZXRpb24gcmVjb3JkLCB3aGljaFxuICAvLyBoYXMgYSBzdGFibGUgc2hhcGUgYW5kIHNvIGhvcGVmdWxseSBzaG91bGQgYmUgY2hlYXAgdG8gYWxsb2NhdGUuXG4gIGZ1bmN0aW9uIHRyeUNhdGNoKGZuLCBvYmosIGFyZykge1xuICAgIHRyeSB7XG4gICAgICByZXR1cm4geyB0eXBlOiBcIm5vcm1hbFwiLCBhcmc6IGZuLmNhbGwob2JqLCBhcmcpIH07XG4gICAgfSBjYXRjaCAoZXJyKSB7XG4gICAgICByZXR1cm4geyB0eXBlOiBcInRocm93XCIsIGFyZzogZXJyIH07XG4gICAgfVxuICB9XG5cbiAgdmFyIEdlblN0YXRlU3VzcGVuZGVkU3RhcnQgPSBcInN1c3BlbmRlZFN0YXJ0XCI7XG4gIHZhciBHZW5TdGF0ZVN1c3BlbmRlZFlpZWxkID0gXCJzdXNwZW5kZWRZaWVsZFwiO1xuICB2YXIgR2VuU3RhdGVFeGVjdXRpbmcgPSBcImV4ZWN1dGluZ1wiO1xuICB2YXIgR2VuU3RhdGVDb21wbGV0ZWQgPSBcImNvbXBsZXRlZFwiO1xuXG4gIC8vIFJldHVybmluZyB0aGlzIG9iamVjdCBmcm9tIHRoZSBpbm5lckZuIGhhcyB0aGUgc2FtZSBlZmZlY3QgYXNcbiAgLy8gYnJlYWtpbmcgb3V0IG9mIHRoZSBkaXNwYXRjaCBzd2l0Y2ggc3RhdGVtZW50LlxuICB2YXIgQ29udGludWVTZW50aW5lbCA9IHt9O1xuXG4gIC8vIER1bW15IGNvbnN0cnVjdG9yIGZ1bmN0aW9ucyB0aGF0IHdlIHVzZSBhcyB0aGUgLmNvbnN0cnVjdG9yIGFuZFxuICAvLyAuY29uc3RydWN0b3IucHJvdG90eXBlIHByb3BlcnRpZXMgZm9yIGZ1bmN0aW9ucyB0aGF0IHJldHVybiBHZW5lcmF0b3JcbiAgLy8gb2JqZWN0cy4gRm9yIGZ1bGwgc3BlYyBjb21wbGlhbmNlLCB5b3UgbWF5IHdpc2ggdG8gY29uZmlndXJlIHlvdXJcbiAgLy8gbWluaWZpZXIgbm90IHRvIG1hbmdsZSB0aGUgbmFtZXMgb2YgdGhlc2UgdHdvIGZ1bmN0aW9ucy5cbiAgZnVuY3Rpb24gR2VuZXJhdG9yKCkge31cbiAgZnVuY3Rpb24gR2VuZXJhdG9yRnVuY3Rpb24oKSB7fVxuICBmdW5jdGlvbiBHZW5lcmF0b3JGdW5jdGlvblByb3RvdHlwZSgpIHt9XG5cbiAgLy8gVGhpcyBpcyBhIHBvbHlmaWxsIGZvciAlSXRlcmF0b3JQcm90b3R5cGUlIGZvciBlbnZpcm9ubWVudHMgdGhhdFxuICAvLyBkb24ndCBuYXRpdmVseSBzdXBwb3J0IGl0LlxuICB2YXIgSXRlcmF0b3JQcm90b3R5cGUgPSB7fTtcbiAgSXRlcmF0b3JQcm90b3R5cGVbaXRlcmF0b3JTeW1ib2xdID0gZnVuY3Rpb24gKCkge1xuICAgIHJldHVybiB0aGlzO1xuICB9O1xuXG4gIHZhciBnZXRQcm90byA9IE9iamVjdC5nZXRQcm90b3R5cGVPZjtcbiAgdmFyIE5hdGl2ZUl0ZXJhdG9yUHJvdG90eXBlID0gZ2V0UHJvdG8gJiYgZ2V0UHJvdG8oZ2V0UHJvdG8odmFsdWVzKFtdKSkpO1xuICBpZiAoTmF0aXZlSXRlcmF0b3JQcm90b3R5cGUgJiZcbiAgICAgIE5hdGl2ZUl0ZXJhdG9yUHJvdG90eXBlICE9PSBPcCAmJlxuICAgICAgaGFzT3duLmNhbGwoTmF0aXZlSXRlcmF0b3JQcm90b3R5cGUsIGl0ZXJhdG9yU3ltYm9sKSkge1xuICAgIC8vIFRoaXMgZW52aXJvbm1lbnQgaGFzIGEgbmF0aXZlICVJdGVyYXRvclByb3RvdHlwZSU7IHVzZSBpdCBpbnN0ZWFkXG4gICAgLy8gb2YgdGhlIHBvbHlmaWxsLlxuICAgIEl0ZXJhdG9yUHJvdG90eXBlID0gTmF0aXZlSXRlcmF0b3JQcm90b3R5cGU7XG4gIH1cblxuICB2YXIgR3AgPSBHZW5lcmF0b3JGdW5jdGlvblByb3RvdHlwZS5wcm90b3R5cGUgPVxuICAgIEdlbmVyYXRvci5wcm90b3R5cGUgPSBPYmplY3QuY3JlYXRlKEl0ZXJhdG9yUHJvdG90eXBlKTtcbiAgR2VuZXJhdG9yRnVuY3Rpb24ucHJvdG90eXBlID0gR3AuY29uc3RydWN0b3IgPSBHZW5lcmF0b3JGdW5jdGlvblByb3RvdHlwZTtcbiAgR2VuZXJhdG9yRnVuY3Rpb25Qcm90b3R5cGUuY29uc3RydWN0b3IgPSBHZW5lcmF0b3JGdW5jdGlvbjtcbiAgR2VuZXJhdG9yRnVuY3Rpb24uZGlzcGxheU5hbWUgPSBkZWZpbmUoXG4gICAgR2VuZXJhdG9yRnVuY3Rpb25Qcm90b3R5cGUsXG4gICAgdG9TdHJpbmdUYWdTeW1ib2wsXG4gICAgXCJHZW5lcmF0b3JGdW5jdGlvblwiXG4gICk7XG5cbiAgLy8gSGVscGVyIGZvciBkZWZpbmluZyB0aGUgLm5leHQsIC50aHJvdywgYW5kIC5yZXR1cm4gbWV0aG9kcyBvZiB0aGVcbiAgLy8gSXRlcmF0b3IgaW50ZXJmYWNlIGluIHRlcm1zIG9mIGEgc2luZ2xlIC5faW52b2tlIG1ldGhvZC5cbiAgZnVuY3Rpb24gZGVmaW5lSXRlcmF0b3JNZXRob2RzKHByb3RvdHlwZSkge1xuICAgIFtcIm5leHRcIiwgXCJ0aHJvd1wiLCBcInJldHVyblwiXS5mb3JFYWNoKGZ1bmN0aW9uKG1ldGhvZCkge1xuICAgICAgZGVmaW5lKHByb3RvdHlwZSwgbWV0aG9kLCBmdW5jdGlvbihhcmcpIHtcbiAgICAgICAgcmV0dXJuIHRoaXMuX2ludm9rZShtZXRob2QsIGFyZyk7XG4gICAgICB9KTtcbiAgICB9KTtcbiAgfVxuXG4gIGV4cG9ydHMuaXNHZW5lcmF0b3JGdW5jdGlvbiA9IGZ1bmN0aW9uKGdlbkZ1bikge1xuICAgIHZhciBjdG9yID0gdHlwZW9mIGdlbkZ1biA9PT0gXCJmdW5jdGlvblwiICYmIGdlbkZ1bi5jb25zdHJ1Y3RvcjtcbiAgICByZXR1cm4gY3RvclxuICAgICAgPyBjdG9yID09PSBHZW5lcmF0b3JGdW5jdGlvbiB8fFxuICAgICAgICAvLyBGb3IgdGhlIG5hdGl2ZSBHZW5lcmF0b3JGdW5jdGlvbiBjb25zdHJ1Y3RvciwgdGhlIGJlc3Qgd2UgY2FuXG4gICAgICAgIC8vIGRvIGlzIHRvIGNoZWNrIGl0cyAubmFtZSBwcm9wZXJ0eS5cbiAgICAgICAgKGN0b3IuZGlzcGxheU5hbWUgfHwgY3Rvci5uYW1lKSA9PT0gXCJHZW5lcmF0b3JGdW5jdGlvblwiXG4gICAgICA6IGZhbHNlO1xuICB9O1xuXG4gIGV4cG9ydHMubWFyayA9IGZ1bmN0aW9uKGdlbkZ1bikge1xuICAgIGlmIChPYmplY3Quc2V0UHJvdG90eXBlT2YpIHtcbiAgICAgIE9iamVjdC5zZXRQcm90b3R5cGVPZihnZW5GdW4sIEdlbmVyYXRvckZ1bmN0aW9uUHJvdG90eXBlKTtcbiAgICB9IGVsc2Uge1xuICAgICAgZ2VuRnVuLl9fcHJvdG9fXyA9IEdlbmVyYXRvckZ1bmN0aW9uUHJvdG90eXBlO1xuICAgICAgZGVmaW5lKGdlbkZ1biwgdG9TdHJpbmdUYWdTeW1ib2wsIFwiR2VuZXJhdG9yRnVuY3Rpb25cIik7XG4gICAgfVxuICAgIGdlbkZ1bi5wcm90b3R5cGUgPSBPYmplY3QuY3JlYXRlKEdwKTtcbiAgICByZXR1cm4gZ2VuRnVuO1xuICB9O1xuXG4gIC8vIFdpdGhpbiB0aGUgYm9keSBvZiBhbnkgYXN5bmMgZnVuY3Rpb24sIGBhd2FpdCB4YCBpcyB0cmFuc2Zvcm1lZCB0b1xuICAvLyBgeWllbGQgcmVnZW5lcmF0b3JSdW50aW1lLmF3cmFwKHgpYCwgc28gdGhhdCB0aGUgcnVudGltZSBjYW4gdGVzdFxuICAvLyBgaGFzT3duLmNhbGwodmFsdWUsIFwiX19hd2FpdFwiKWAgdG8gZGV0ZXJtaW5lIGlmIHRoZSB5aWVsZGVkIHZhbHVlIGlzXG4gIC8vIG1lYW50IHRvIGJlIGF3YWl0ZWQuXG4gIGV4cG9ydHMuYXdyYXAgPSBmdW5jdGlvbihhcmcpIHtcbiAgICByZXR1cm4geyBfX2F3YWl0OiBhcmcgfTtcbiAgfTtcblxuICBmdW5jdGlvbiBBc3luY0l0ZXJhdG9yKGdlbmVyYXRvciwgUHJvbWlzZUltcGwpIHtcbiAgICBmdW5jdGlvbiBpbnZva2UobWV0aG9kLCBhcmcsIHJlc29sdmUsIHJlamVjdCkge1xuICAgICAgdmFyIHJlY29yZCA9IHRyeUNhdGNoKGdlbmVyYXRvclttZXRob2RdLCBnZW5lcmF0b3IsIGFyZyk7XG4gICAgICBpZiAocmVjb3JkLnR5cGUgPT09IFwidGhyb3dcIikge1xuICAgICAgICByZWplY3QocmVjb3JkLmFyZyk7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICB2YXIgcmVzdWx0ID0gcmVjb3JkLmFyZztcbiAgICAgICAgdmFyIHZhbHVlID0gcmVzdWx0LnZhbHVlO1xuICAgICAgICBpZiAodmFsdWUgJiZcbiAgICAgICAgICAgIHR5cGVvZiB2YWx1ZSA9PT0gXCJvYmplY3RcIiAmJlxuICAgICAgICAgICAgaGFzT3duLmNhbGwodmFsdWUsIFwiX19hd2FpdFwiKSkge1xuICAgICAgICAgIHJldHVybiBQcm9taXNlSW1wbC5yZXNvbHZlKHZhbHVlLl9fYXdhaXQpLnRoZW4oZnVuY3Rpb24odmFsdWUpIHtcbiAgICAgICAgICAgIGludm9rZShcIm5leHRcIiwgdmFsdWUsIHJlc29sdmUsIHJlamVjdCk7XG4gICAgICAgICAgfSwgZnVuY3Rpb24oZXJyKSB7XG4gICAgICAgICAgICBpbnZva2UoXCJ0aHJvd1wiLCBlcnIsIHJlc29sdmUsIHJlamVjdCk7XG4gICAgICAgICAgfSk7XG4gICAgICAgIH1cblxuICAgICAgICByZXR1cm4gUHJvbWlzZUltcGwucmVzb2x2ZSh2YWx1ZSkudGhlbihmdW5jdGlvbih1bndyYXBwZWQpIHtcbiAgICAgICAgICAvLyBXaGVuIGEgeWllbGRlZCBQcm9taXNlIGlzIHJlc29sdmVkLCBpdHMgZmluYWwgdmFsdWUgYmVjb21lc1xuICAgICAgICAgIC8vIHRoZSAudmFsdWUgb2YgdGhlIFByb21pc2U8e3ZhbHVlLGRvbmV9PiByZXN1bHQgZm9yIHRoZVxuICAgICAgICAgIC8vIGN1cnJlbnQgaXRlcmF0aW9uLlxuICAgICAgICAgIHJlc3VsdC52YWx1ZSA9IHVud3JhcHBlZDtcbiAgICAgICAgICByZXNvbHZlKHJlc3VsdCk7XG4gICAgICAgIH0sIGZ1bmN0aW9uKGVycm9yKSB7XG4gICAgICAgICAgLy8gSWYgYSByZWplY3RlZCBQcm9taXNlIHdhcyB5aWVsZGVkLCB0aHJvdyB0aGUgcmVqZWN0aW9uIGJhY2tcbiAgICAgICAgICAvLyBpbnRvIHRoZSBhc3luYyBnZW5lcmF0b3IgZnVuY3Rpb24gc28gaXQgY2FuIGJlIGhhbmRsZWQgdGhlcmUuXG4gICAgICAgICAgcmV0dXJuIGludm9rZShcInRocm93XCIsIGVycm9yLCByZXNvbHZlLCByZWplY3QpO1xuICAgICAgICB9KTtcbiAgICAgIH1cbiAgICB9XG5cbiAgICB2YXIgcHJldmlvdXNQcm9taXNlO1xuXG4gICAgZnVuY3Rpb24gZW5xdWV1ZShtZXRob2QsIGFyZykge1xuICAgICAgZnVuY3Rpb24gY2FsbEludm9rZVdpdGhNZXRob2RBbmRBcmcoKSB7XG4gICAgICAgIHJldHVybiBuZXcgUHJvbWlzZUltcGwoZnVuY3Rpb24ocmVzb2x2ZSwgcmVqZWN0KSB7XG4gICAgICAgICAgaW52b2tlKG1ldGhvZCwgYXJnLCByZXNvbHZlLCByZWplY3QpO1xuICAgICAgICB9KTtcbiAgICAgIH1cblxuICAgICAgcmV0dXJuIHByZXZpb3VzUHJvbWlzZSA9XG4gICAgICAgIC8vIElmIGVucXVldWUgaGFzIGJlZW4gY2FsbGVkIGJlZm9yZSwgdGhlbiB3ZSB3YW50IHRvIHdhaXQgdW50aWxcbiAgICAgICAgLy8gYWxsIHByZXZpb3VzIFByb21pc2VzIGhhdmUgYmVlbiByZXNvbHZlZCBiZWZvcmUgY2FsbGluZyBpbnZva2UsXG4gICAgICAgIC8vIHNvIHRoYXQgcmVzdWx0cyBhcmUgYWx3YXlzIGRlbGl2ZXJlZCBpbiB0aGUgY29ycmVjdCBvcmRlci4gSWZcbiAgICAgICAgLy8gZW5xdWV1ZSBoYXMgbm90IGJlZW4gY2FsbGVkIGJlZm9yZSwgdGhlbiBpdCBpcyBpbXBvcnRhbnQgdG9cbiAgICAgICAgLy8gY2FsbCBpbnZva2UgaW1tZWRpYXRlbHksIHdpdGhvdXQgd2FpdGluZyBvbiBhIGNhbGxiYWNrIHRvIGZpcmUsXG4gICAgICAgIC8vIHNvIHRoYXQgdGhlIGFzeW5jIGdlbmVyYXRvciBmdW5jdGlvbiBoYXMgdGhlIG9wcG9ydHVuaXR5IHRvIGRvXG4gICAgICAgIC8vIGFueSBuZWNlc3Nhcnkgc2V0dXAgaW4gYSBwcmVkaWN0YWJsZSB3YXkuIFRoaXMgcHJlZGljdGFiaWxpdHlcbiAgICAgICAgLy8gaXMgd2h5IHRoZSBQcm9taXNlIGNvbnN0cnVjdG9yIHN5bmNocm9ub3VzbHkgaW52b2tlcyBpdHNcbiAgICAgICAgLy8gZXhlY3V0b3IgY2FsbGJhY2ssIGFuZCB3aHkgYXN5bmMgZnVuY3Rpb25zIHN5bmNocm9ub3VzbHlcbiAgICAgICAgLy8gZXhlY3V0ZSBjb2RlIGJlZm9yZSB0aGUgZmlyc3QgYXdhaXQuIFNpbmNlIHdlIGltcGxlbWVudCBzaW1wbGVcbiAgICAgICAgLy8gYXN5bmMgZnVuY3Rpb25zIGluIHRlcm1zIG9mIGFzeW5jIGdlbmVyYXRvcnMsIGl0IGlzIGVzcGVjaWFsbHlcbiAgICAgICAgLy8gaW1wb3J0YW50IHRvIGdldCB0aGlzIHJpZ2h0LCBldmVuIHRob3VnaCBpdCByZXF1aXJlcyBjYXJlLlxuICAgICAgICBwcmV2aW91c1Byb21pc2UgPyBwcmV2aW91c1Byb21pc2UudGhlbihcbiAgICAgICAgICBjYWxsSW52b2tlV2l0aE1ldGhvZEFuZEFyZyxcbiAgICAgICAgICAvLyBBdm9pZCBwcm9wYWdhdGluZyBmYWlsdXJlcyB0byBQcm9taXNlcyByZXR1cm5lZCBieSBsYXRlclxuICAgICAgICAgIC8vIGludm9jYXRpb25zIG9mIHRoZSBpdGVyYXRvci5cbiAgICAgICAgICBjYWxsSW52b2tlV2l0aE1ldGhvZEFuZEFyZ1xuICAgICAgICApIDogY2FsbEludm9rZVdpdGhNZXRob2RBbmRBcmcoKTtcbiAgICB9XG5cbiAgICAvLyBEZWZpbmUgdGhlIHVuaWZpZWQgaGVscGVyIG1ldGhvZCB0aGF0IGlzIHVzZWQgdG8gaW1wbGVtZW50IC5uZXh0LFxuICAgIC8vIC50aHJvdywgYW5kIC5yZXR1cm4gKHNlZSBkZWZpbmVJdGVyYXRvck1ldGhvZHMpLlxuICAgIHRoaXMuX2ludm9rZSA9IGVucXVldWU7XG4gIH1cblxuICBkZWZpbmVJdGVyYXRvck1ldGhvZHMoQXN5bmNJdGVyYXRvci5wcm90b3R5cGUpO1xuICBBc3luY0l0ZXJhdG9yLnByb3RvdHlwZVthc3luY0l0ZXJhdG9yU3ltYm9sXSA9IGZ1bmN0aW9uICgpIHtcbiAgICByZXR1cm4gdGhpcztcbiAgfTtcbiAgZXhwb3J0cy5Bc3luY0l0ZXJhdG9yID0gQXN5bmNJdGVyYXRvcjtcblxuICAvLyBOb3RlIHRoYXQgc2ltcGxlIGFzeW5jIGZ1bmN0aW9ucyBhcmUgaW1wbGVtZW50ZWQgb24gdG9wIG9mXG4gIC8vIEFzeW5jSXRlcmF0b3Igb2JqZWN0czsgdGhleSBqdXN0IHJldHVybiBhIFByb21pc2UgZm9yIHRoZSB2YWx1ZSBvZlxuICAvLyB0aGUgZmluYWwgcmVzdWx0IHByb2R1Y2VkIGJ5IHRoZSBpdGVyYXRvci5cbiAgZXhwb3J0cy5hc3luYyA9IGZ1bmN0aW9uKGlubmVyRm4sIG91dGVyRm4sIHNlbGYsIHRyeUxvY3NMaXN0LCBQcm9taXNlSW1wbCkge1xuICAgIGlmIChQcm9taXNlSW1wbCA9PT0gdm9pZCAwKSBQcm9taXNlSW1wbCA9IFByb21pc2U7XG5cbiAgICB2YXIgaXRlciA9IG5ldyBBc3luY0l0ZXJhdG9yKFxuICAgICAgd3JhcChpbm5lckZuLCBvdXRlckZuLCBzZWxmLCB0cnlMb2NzTGlzdCksXG4gICAgICBQcm9taXNlSW1wbFxuICAgICk7XG5cbiAgICByZXR1cm4gZXhwb3J0cy5pc0dlbmVyYXRvckZ1bmN0aW9uKG91dGVyRm4pXG4gICAgICA/IGl0ZXIgLy8gSWYgb3V0ZXJGbiBpcyBhIGdlbmVyYXRvciwgcmV0dXJuIHRoZSBmdWxsIGl0ZXJhdG9yLlxuICAgICAgOiBpdGVyLm5leHQoKS50aGVuKGZ1bmN0aW9uKHJlc3VsdCkge1xuICAgICAgICAgIHJldHVybiByZXN1bHQuZG9uZSA/IHJlc3VsdC52YWx1ZSA6IGl0ZXIubmV4dCgpO1xuICAgICAgICB9KTtcbiAgfTtcblxuICBmdW5jdGlvbiBtYWtlSW52b2tlTWV0aG9kKGlubmVyRm4sIHNlbGYsIGNvbnRleHQpIHtcbiAgICB2YXIgc3RhdGUgPSBHZW5TdGF0ZVN1c3BlbmRlZFN0YXJ0O1xuXG4gICAgcmV0dXJuIGZ1bmN0aW9uIGludm9rZShtZXRob2QsIGFyZykge1xuICAgICAgaWYgKHN0YXRlID09PSBHZW5TdGF0ZUV4ZWN1dGluZykge1xuICAgICAgICB0aHJvdyBuZXcgRXJyb3IoXCJHZW5lcmF0b3IgaXMgYWxyZWFkeSBydW5uaW5nXCIpO1xuICAgICAgfVxuXG4gICAgICBpZiAoc3RhdGUgPT09IEdlblN0YXRlQ29tcGxldGVkKSB7XG4gICAgICAgIGlmIChtZXRob2QgPT09IFwidGhyb3dcIikge1xuICAgICAgICAgIHRocm93IGFyZztcbiAgICAgICAgfVxuXG4gICAgICAgIC8vIEJlIGZvcmdpdmluZywgcGVyIDI1LjMuMy4zLjMgb2YgdGhlIHNwZWM6XG4gICAgICAgIC8vIGh0dHBzOi8vcGVvcGxlLm1vemlsbGEub3JnL35qb3JlbmRvcmZmL2VzNi1kcmFmdC5odG1sI3NlYy1nZW5lcmF0b3JyZXN1bWVcbiAgICAgICAgcmV0dXJuIGRvbmVSZXN1bHQoKTtcbiAgICAgIH1cblxuICAgICAgY29udGV4dC5tZXRob2QgPSBtZXRob2Q7XG4gICAgICBjb250ZXh0LmFyZyA9IGFyZztcblxuICAgICAgd2hpbGUgKHRydWUpIHtcbiAgICAgICAgdmFyIGRlbGVnYXRlID0gY29udGV4dC5kZWxlZ2F0ZTtcbiAgICAgICAgaWYgKGRlbGVnYXRlKSB7XG4gICAgICAgICAgdmFyIGRlbGVnYXRlUmVzdWx0ID0gbWF5YmVJbnZva2VEZWxlZ2F0ZShkZWxlZ2F0ZSwgY29udGV4dCk7XG4gICAgICAgICAgaWYgKGRlbGVnYXRlUmVzdWx0KSB7XG4gICAgICAgICAgICBpZiAoZGVsZWdhdGVSZXN1bHQgPT09IENvbnRpbnVlU2VudGluZWwpIGNvbnRpbnVlO1xuICAgICAgICAgICAgcmV0dXJuIGRlbGVnYXRlUmVzdWx0O1xuICAgICAgICAgIH1cbiAgICAgICAgfVxuXG4gICAgICAgIGlmIChjb250ZXh0Lm1ldGhvZCA9PT0gXCJuZXh0XCIpIHtcbiAgICAgICAgICAvLyBTZXR0aW5nIGNvbnRleHQuX3NlbnQgZm9yIGxlZ2FjeSBzdXBwb3J0IG9mIEJhYmVsJ3NcbiAgICAgICAgICAvLyBmdW5jdGlvbi5zZW50IGltcGxlbWVudGF0aW9uLlxuICAgICAgICAgIGNvbnRleHQuc2VudCA9IGNvbnRleHQuX3NlbnQgPSBjb250ZXh0LmFyZztcblxuICAgICAgICB9IGVsc2UgaWYgKGNvbnRleHQubWV0aG9kID09PSBcInRocm93XCIpIHtcbiAgICAgICAgICBpZiAoc3RhdGUgPT09IEdlblN0YXRlU3VzcGVuZGVkU3RhcnQpIHtcbiAgICAgICAgICAgIHN0YXRlID0gR2VuU3RhdGVDb21wbGV0ZWQ7XG4gICAgICAgICAgICB0aHJvdyBjb250ZXh0LmFyZztcbiAgICAgICAgICB9XG5cbiAgICAgICAgICBjb250ZXh0LmRpc3BhdGNoRXhjZXB0aW9uKGNvbnRleHQuYXJnKTtcblxuICAgICAgICB9IGVsc2UgaWYgKGNvbnRleHQubWV0aG9kID09PSBcInJldHVyblwiKSB7XG4gICAgICAgICAgY29udGV4dC5hYnJ1cHQoXCJyZXR1cm5cIiwgY29udGV4dC5hcmcpO1xuICAgICAgICB9XG5cbiAgICAgICAgc3RhdGUgPSBHZW5TdGF0ZUV4ZWN1dGluZztcblxuICAgICAgICB2YXIgcmVjb3JkID0gdHJ5Q2F0Y2goaW5uZXJGbiwgc2VsZiwgY29udGV4dCk7XG4gICAgICAgIGlmIChyZWNvcmQudHlwZSA9PT0gXCJub3JtYWxcIikge1xuICAgICAgICAgIC8vIElmIGFuIGV4Y2VwdGlvbiBpcyB0aHJvd24gZnJvbSBpbm5lckZuLCB3ZSBsZWF2ZSBzdGF0ZSA9PT1cbiAgICAgICAgICAvLyBHZW5TdGF0ZUV4ZWN1dGluZyBhbmQgbG9vcCBiYWNrIGZvciBhbm90aGVyIGludm9jYXRpb24uXG4gICAgICAgICAgc3RhdGUgPSBjb250ZXh0LmRvbmVcbiAgICAgICAgICAgID8gR2VuU3RhdGVDb21wbGV0ZWRcbiAgICAgICAgICAgIDogR2VuU3RhdGVTdXNwZW5kZWRZaWVsZDtcblxuICAgICAgICAgIGlmIChyZWNvcmQuYXJnID09PSBDb250aW51ZVNlbnRpbmVsKSB7XG4gICAgICAgICAgICBjb250aW51ZTtcbiAgICAgICAgICB9XG5cbiAgICAgICAgICByZXR1cm4ge1xuICAgICAgICAgICAgdmFsdWU6IHJlY29yZC5hcmcsXG4gICAgICAgICAgICBkb25lOiBjb250ZXh0LmRvbmVcbiAgICAgICAgICB9O1xuXG4gICAgICAgIH0gZWxzZSBpZiAocmVjb3JkLnR5cGUgPT09IFwidGhyb3dcIikge1xuICAgICAgICAgIHN0YXRlID0gR2VuU3RhdGVDb21wbGV0ZWQ7XG4gICAgICAgICAgLy8gRGlzcGF0Y2ggdGhlIGV4Y2VwdGlvbiBieSBsb29waW5nIGJhY2sgYXJvdW5kIHRvIHRoZVxuICAgICAgICAgIC8vIGNvbnRleHQuZGlzcGF0Y2hFeGNlcHRpb24oY29udGV4dC5hcmcpIGNhbGwgYWJvdmUuXG4gICAgICAgICAgY29udGV4dC5tZXRob2QgPSBcInRocm93XCI7XG4gICAgICAgICAgY29udGV4dC5hcmcgPSByZWNvcmQuYXJnO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgfTtcbiAgfVxuXG4gIC8vIENhbGwgZGVsZWdhdGUuaXRlcmF0b3JbY29udGV4dC5tZXRob2RdKGNvbnRleHQuYXJnKSBhbmQgaGFuZGxlIHRoZVxuICAvLyByZXN1bHQsIGVpdGhlciBieSByZXR1cm5pbmcgYSB7IHZhbHVlLCBkb25lIH0gcmVzdWx0IGZyb20gdGhlXG4gIC8vIGRlbGVnYXRlIGl0ZXJhdG9yLCBvciBieSBtb2RpZnlpbmcgY29udGV4dC5tZXRob2QgYW5kIGNvbnRleHQuYXJnLFxuICAvLyBzZXR0aW5nIGNvbnRleHQuZGVsZWdhdGUgdG8gbnVsbCwgYW5kIHJldHVybmluZyB0aGUgQ29udGludWVTZW50aW5lbC5cbiAgZnVuY3Rpb24gbWF5YmVJbnZva2VEZWxlZ2F0ZShkZWxlZ2F0ZSwgY29udGV4dCkge1xuICAgIHZhciBtZXRob2QgPSBkZWxlZ2F0ZS5pdGVyYXRvcltjb250ZXh0Lm1ldGhvZF07XG4gICAgaWYgKG1ldGhvZCA9PT0gdW5kZWZpbmVkKSB7XG4gICAgICAvLyBBIC50aHJvdyBvciAucmV0dXJuIHdoZW4gdGhlIGRlbGVnYXRlIGl0ZXJhdG9yIGhhcyBubyAudGhyb3dcbiAgICAgIC8vIG1ldGhvZCBhbHdheXMgdGVybWluYXRlcyB0aGUgeWllbGQqIGxvb3AuXG4gICAgICBjb250ZXh0LmRlbGVnYXRlID0gbnVsbDtcblxuICAgICAgaWYgKGNvbnRleHQubWV0aG9kID09PSBcInRocm93XCIpIHtcbiAgICAgICAgLy8gTm90ZTogW1wicmV0dXJuXCJdIG11c3QgYmUgdXNlZCBmb3IgRVMzIHBhcnNpbmcgY29tcGF0aWJpbGl0eS5cbiAgICAgICAgaWYgKGRlbGVnYXRlLml0ZXJhdG9yW1wicmV0dXJuXCJdKSB7XG4gICAgICAgICAgLy8gSWYgdGhlIGRlbGVnYXRlIGl0ZXJhdG9yIGhhcyBhIHJldHVybiBtZXRob2QsIGdpdmUgaXQgYVxuICAgICAgICAgIC8vIGNoYW5jZSB0byBjbGVhbiB1cC5cbiAgICAgICAgICBjb250ZXh0Lm1ldGhvZCA9IFwicmV0dXJuXCI7XG4gICAgICAgICAgY29udGV4dC5hcmcgPSB1bmRlZmluZWQ7XG4gICAgICAgICAgbWF5YmVJbnZva2VEZWxlZ2F0ZShkZWxlZ2F0ZSwgY29udGV4dCk7XG5cbiAgICAgICAgICBpZiAoY29udGV4dC5tZXRob2QgPT09IFwidGhyb3dcIikge1xuICAgICAgICAgICAgLy8gSWYgbWF5YmVJbnZva2VEZWxlZ2F0ZShjb250ZXh0KSBjaGFuZ2VkIGNvbnRleHQubWV0aG9kIGZyb21cbiAgICAgICAgICAgIC8vIFwicmV0dXJuXCIgdG8gXCJ0aHJvd1wiLCBsZXQgdGhhdCBvdmVycmlkZSB0aGUgVHlwZUVycm9yIGJlbG93LlxuICAgICAgICAgICAgcmV0dXJuIENvbnRpbnVlU2VudGluZWw7XG4gICAgICAgICAgfVxuICAgICAgICB9XG5cbiAgICAgICAgY29udGV4dC5tZXRob2QgPSBcInRocm93XCI7XG4gICAgICAgIGNvbnRleHQuYXJnID0gbmV3IFR5cGVFcnJvcihcbiAgICAgICAgICBcIlRoZSBpdGVyYXRvciBkb2VzIG5vdCBwcm92aWRlIGEgJ3Rocm93JyBtZXRob2RcIik7XG4gICAgICB9XG5cbiAgICAgIHJldHVybiBDb250aW51ZVNlbnRpbmVsO1xuICAgIH1cblxuICAgIHZhciByZWNvcmQgPSB0cnlDYXRjaChtZXRob2QsIGRlbGVnYXRlLml0ZXJhdG9yLCBjb250ZXh0LmFyZyk7XG5cbiAgICBpZiAocmVjb3JkLnR5cGUgPT09IFwidGhyb3dcIikge1xuICAgICAgY29udGV4dC5tZXRob2QgPSBcInRocm93XCI7XG4gICAgICBjb250ZXh0LmFyZyA9IHJlY29yZC5hcmc7XG4gICAgICBjb250ZXh0LmRlbGVnYXRlID0gbnVsbDtcbiAgICAgIHJldHVybiBDb250aW51ZVNlbnRpbmVsO1xuICAgIH1cblxuICAgIHZhciBpbmZvID0gcmVjb3JkLmFyZztcblxuICAgIGlmICghIGluZm8pIHtcbiAgICAgIGNvbnRleHQubWV0aG9kID0gXCJ0aHJvd1wiO1xuICAgICAgY29udGV4dC5hcmcgPSBuZXcgVHlwZUVycm9yKFwiaXRlcmF0b3IgcmVzdWx0IGlzIG5vdCBhbiBvYmplY3RcIik7XG4gICAgICBjb250ZXh0LmRlbGVnYXRlID0gbnVsbDtcbiAgICAgIHJldHVybiBDb250aW51ZVNlbnRpbmVsO1xuICAgIH1cblxuICAgIGlmIChpbmZvLmRvbmUpIHtcbiAgICAgIC8vIEFzc2lnbiB0aGUgcmVzdWx0IG9mIHRoZSBmaW5pc2hlZCBkZWxlZ2F0ZSB0byB0aGUgdGVtcG9yYXJ5XG4gICAgICAvLyB2YXJpYWJsZSBzcGVjaWZpZWQgYnkgZGVsZWdhdGUucmVzdWx0TmFtZSAoc2VlIGRlbGVnYXRlWWllbGQpLlxuICAgICAgY29udGV4dFtkZWxlZ2F0ZS5yZXN1bHROYW1lXSA9IGluZm8udmFsdWU7XG5cbiAgICAgIC8vIFJlc3VtZSBleGVjdXRpb24gYXQgdGhlIGRlc2lyZWQgbG9jYXRpb24gKHNlZSBkZWxlZ2F0ZVlpZWxkKS5cbiAgICAgIGNvbnRleHQubmV4dCA9IGRlbGVnYXRlLm5leHRMb2M7XG5cbiAgICAgIC8vIElmIGNvbnRleHQubWV0aG9kIHdhcyBcInRocm93XCIgYnV0IHRoZSBkZWxlZ2F0ZSBoYW5kbGVkIHRoZVxuICAgICAgLy8gZXhjZXB0aW9uLCBsZXQgdGhlIG91dGVyIGdlbmVyYXRvciBwcm9jZWVkIG5vcm1hbGx5LiBJZlxuICAgICAgLy8gY29udGV4dC5tZXRob2Qgd2FzIFwibmV4dFwiLCBmb3JnZXQgY29udGV4dC5hcmcgc2luY2UgaXQgaGFzIGJlZW5cbiAgICAgIC8vIFwiY29uc3VtZWRcIiBieSB0aGUgZGVsZWdhdGUgaXRlcmF0b3IuIElmIGNvbnRleHQubWV0aG9kIHdhc1xuICAgICAgLy8gXCJyZXR1cm5cIiwgYWxsb3cgdGhlIG9yaWdpbmFsIC5yZXR1cm4gY2FsbCB0byBjb250aW51ZSBpbiB0aGVcbiAgICAgIC8vIG91dGVyIGdlbmVyYXRvci5cbiAgICAgIGlmIChjb250ZXh0Lm1ldGhvZCAhPT0gXCJyZXR1cm5cIikge1xuICAgICAgICBjb250ZXh0Lm1ldGhvZCA9IFwibmV4dFwiO1xuICAgICAgICBjb250ZXh0LmFyZyA9IHVuZGVmaW5lZDtcbiAgICAgIH1cblxuICAgIH0gZWxzZSB7XG4gICAgICAvLyBSZS15aWVsZCB0aGUgcmVzdWx0IHJldHVybmVkIGJ5IHRoZSBkZWxlZ2F0ZSBtZXRob2QuXG4gICAgICByZXR1cm4gaW5mbztcbiAgICB9XG5cbiAgICAvLyBUaGUgZGVsZWdhdGUgaXRlcmF0b3IgaXMgZmluaXNoZWQsIHNvIGZvcmdldCBpdCBhbmQgY29udGludWUgd2l0aFxuICAgIC8vIHRoZSBvdXRlciBnZW5lcmF0b3IuXG4gICAgY29udGV4dC5kZWxlZ2F0ZSA9IG51bGw7XG4gICAgcmV0dXJuIENvbnRpbnVlU2VudGluZWw7XG4gIH1cblxuICAvLyBEZWZpbmUgR2VuZXJhdG9yLnByb3RvdHlwZS57bmV4dCx0aHJvdyxyZXR1cm59IGluIHRlcm1zIG9mIHRoZVxuICAvLyB1bmlmaWVkIC5faW52b2tlIGhlbHBlciBtZXRob2QuXG4gIGRlZmluZUl0ZXJhdG9yTWV0aG9kcyhHcCk7XG5cbiAgZGVmaW5lKEdwLCB0b1N0cmluZ1RhZ1N5bWJvbCwgXCJHZW5lcmF0b3JcIik7XG5cbiAgLy8gQSBHZW5lcmF0b3Igc2hvdWxkIGFsd2F5cyByZXR1cm4gaXRzZWxmIGFzIHRoZSBpdGVyYXRvciBvYmplY3Qgd2hlbiB0aGVcbiAgLy8gQEBpdGVyYXRvciBmdW5jdGlvbiBpcyBjYWxsZWQgb24gaXQuIFNvbWUgYnJvd3NlcnMnIGltcGxlbWVudGF0aW9ucyBvZiB0aGVcbiAgLy8gaXRlcmF0b3IgcHJvdG90eXBlIGNoYWluIGluY29ycmVjdGx5IGltcGxlbWVudCB0aGlzLCBjYXVzaW5nIHRoZSBHZW5lcmF0b3JcbiAgLy8gb2JqZWN0IHRvIG5vdCBiZSByZXR1cm5lZCBmcm9tIHRoaXMgY2FsbC4gVGhpcyBlbnN1cmVzIHRoYXQgZG9lc24ndCBoYXBwZW4uXG4gIC8vIFNlZSBodHRwczovL2dpdGh1Yi5jb20vZmFjZWJvb2svcmVnZW5lcmF0b3IvaXNzdWVzLzI3NCBmb3IgbW9yZSBkZXRhaWxzLlxuICBHcFtpdGVyYXRvclN5bWJvbF0gPSBmdW5jdGlvbigpIHtcbiAgICByZXR1cm4gdGhpcztcbiAgfTtcblxuICBHcC50b1N0cmluZyA9IGZ1bmN0aW9uKCkge1xuICAgIHJldHVybiBcIltvYmplY3QgR2VuZXJhdG9yXVwiO1xuICB9O1xuXG4gIGZ1bmN0aW9uIHB1c2hUcnlFbnRyeShsb2NzKSB7XG4gICAgdmFyIGVudHJ5ID0geyB0cnlMb2M6IGxvY3NbMF0gfTtcblxuICAgIGlmICgxIGluIGxvY3MpIHtcbiAgICAgIGVudHJ5LmNhdGNoTG9jID0gbG9jc1sxXTtcbiAgICB9XG5cbiAgICBpZiAoMiBpbiBsb2NzKSB7XG4gICAgICBlbnRyeS5maW5hbGx5TG9jID0gbG9jc1syXTtcbiAgICAgIGVudHJ5LmFmdGVyTG9jID0gbG9jc1szXTtcbiAgICB9XG5cbiAgICB0aGlzLnRyeUVudHJpZXMucHVzaChlbnRyeSk7XG4gIH1cblxuICBmdW5jdGlvbiByZXNldFRyeUVudHJ5KGVudHJ5KSB7XG4gICAgdmFyIHJlY29yZCA9IGVudHJ5LmNvbXBsZXRpb24gfHwge307XG4gICAgcmVjb3JkLnR5cGUgPSBcIm5vcm1hbFwiO1xuICAgIGRlbGV0ZSByZWNvcmQuYXJnO1xuICAgIGVudHJ5LmNvbXBsZXRpb24gPSByZWNvcmQ7XG4gIH1cblxuICBmdW5jdGlvbiBDb250ZXh0KHRyeUxvY3NMaXN0KSB7XG4gICAgLy8gVGhlIHJvb3QgZW50cnkgb2JqZWN0IChlZmZlY3RpdmVseSBhIHRyeSBzdGF0ZW1lbnQgd2l0aG91dCBhIGNhdGNoXG4gICAgLy8gb3IgYSBmaW5hbGx5IGJsb2NrKSBnaXZlcyB1cyBhIHBsYWNlIHRvIHN0b3JlIHZhbHVlcyB0aHJvd24gZnJvbVxuICAgIC8vIGxvY2F0aW9ucyB3aGVyZSB0aGVyZSBpcyBubyBlbmNsb3NpbmcgdHJ5IHN0YXRlbWVudC5cbiAgICB0aGlzLnRyeUVudHJpZXMgPSBbeyB0cnlMb2M6IFwicm9vdFwiIH1dO1xuICAgIHRyeUxvY3NMaXN0LmZvckVhY2gocHVzaFRyeUVudHJ5LCB0aGlzKTtcbiAgICB0aGlzLnJlc2V0KHRydWUpO1xuICB9XG5cbiAgZXhwb3J0cy5rZXlzID0gZnVuY3Rpb24ob2JqZWN0KSB7XG4gICAgdmFyIGtleXMgPSBbXTtcbiAgICBmb3IgKHZhciBrZXkgaW4gb2JqZWN0KSB7XG4gICAgICBrZXlzLnB1c2goa2V5KTtcbiAgICB9XG4gICAga2V5cy5yZXZlcnNlKCk7XG5cbiAgICAvLyBSYXRoZXIgdGhhbiByZXR1cm5pbmcgYW4gb2JqZWN0IHdpdGggYSBuZXh0IG1ldGhvZCwgd2Uga2VlcFxuICAgIC8vIHRoaW5ncyBzaW1wbGUgYW5kIHJldHVybiB0aGUgbmV4dCBmdW5jdGlvbiBpdHNlbGYuXG4gICAgcmV0dXJuIGZ1bmN0aW9uIG5leHQoKSB7XG4gICAgICB3aGlsZSAoa2V5cy5sZW5ndGgpIHtcbiAgICAgICAgdmFyIGtleSA9IGtleXMucG9wKCk7XG4gICAgICAgIGlmIChrZXkgaW4gb2JqZWN0KSB7XG4gICAgICAgICAgbmV4dC52YWx1ZSA9IGtleTtcbiAgICAgICAgICBuZXh0LmRvbmUgPSBmYWxzZTtcbiAgICAgICAgICByZXR1cm4gbmV4dDtcbiAgICAgICAgfVxuICAgICAgfVxuXG4gICAgICAvLyBUbyBhdm9pZCBjcmVhdGluZyBhbiBhZGRpdGlvbmFsIG9iamVjdCwgd2UganVzdCBoYW5nIHRoZSAudmFsdWVcbiAgICAgIC8vIGFuZCAuZG9uZSBwcm9wZXJ0aWVzIG9mZiB0aGUgbmV4dCBmdW5jdGlvbiBvYmplY3QgaXRzZWxmLiBUaGlzXG4gICAgICAvLyBhbHNvIGVuc3VyZXMgdGhhdCB0aGUgbWluaWZpZXIgd2lsbCBub3QgYW5vbnltaXplIHRoZSBmdW5jdGlvbi5cbiAgICAgIG5leHQuZG9uZSA9IHRydWU7XG4gICAgICByZXR1cm4gbmV4dDtcbiAgICB9O1xuICB9O1xuXG4gIGZ1bmN0aW9uIHZhbHVlcyhpdGVyYWJsZSkge1xuICAgIGlmIChpdGVyYWJsZSkge1xuICAgICAgdmFyIGl0ZXJhdG9yTWV0aG9kID0gaXRlcmFibGVbaXRlcmF0b3JTeW1ib2xdO1xuICAgICAgaWYgKGl0ZXJhdG9yTWV0aG9kKSB7XG4gICAgICAgIHJldHVybiBpdGVyYXRvck1ldGhvZC5jYWxsKGl0ZXJhYmxlKTtcbiAgICAgIH1cblxuICAgICAgaWYgKHR5cGVvZiBpdGVyYWJsZS5uZXh0ID09PSBcImZ1bmN0aW9uXCIpIHtcbiAgICAgICAgcmV0dXJuIGl0ZXJhYmxlO1xuICAgICAgfVxuXG4gICAgICBpZiAoIWlzTmFOKGl0ZXJhYmxlLmxlbmd0aCkpIHtcbiAgICAgICAgdmFyIGkgPSAtMSwgbmV4dCA9IGZ1bmN0aW9uIG5leHQoKSB7XG4gICAgICAgICAgd2hpbGUgKCsraSA8IGl0ZXJhYmxlLmxlbmd0aCkge1xuICAgICAgICAgICAgaWYgKGhhc093bi5jYWxsKGl0ZXJhYmxlLCBpKSkge1xuICAgICAgICAgICAgICBuZXh0LnZhbHVlID0gaXRlcmFibGVbaV07XG4gICAgICAgICAgICAgIG5leHQuZG9uZSA9IGZhbHNlO1xuICAgICAgICAgICAgICByZXR1cm4gbmV4dDtcbiAgICAgICAgICAgIH1cbiAgICAgICAgICB9XG5cbiAgICAgICAgICBuZXh0LnZhbHVlID0gdW5kZWZpbmVkO1xuICAgICAgICAgIG5leHQuZG9uZSA9IHRydWU7XG5cbiAgICAgICAgICByZXR1cm4gbmV4dDtcbiAgICAgICAgfTtcblxuICAgICAgICByZXR1cm4gbmV4dC5uZXh0ID0gbmV4dDtcbiAgICAgIH1cbiAgICB9XG5cbiAgICAvLyBSZXR1cm4gYW4gaXRlcmF0b3Igd2l0aCBubyB2YWx1ZXMuXG4gICAgcmV0dXJuIHsgbmV4dDogZG9uZVJlc3VsdCB9O1xuICB9XG4gIGV4cG9ydHMudmFsdWVzID0gdmFsdWVzO1xuXG4gIGZ1bmN0aW9uIGRvbmVSZXN1bHQoKSB7XG4gICAgcmV0dXJuIHsgdmFsdWU6IHVuZGVmaW5lZCwgZG9uZTogdHJ1ZSB9O1xuICB9XG5cbiAgQ29udGV4dC5wcm90b3R5cGUgPSB7XG4gICAgY29uc3RydWN0b3I6IENvbnRleHQsXG5cbiAgICByZXNldDogZnVuY3Rpb24oc2tpcFRlbXBSZXNldCkge1xuICAgICAgdGhpcy5wcmV2ID0gMDtcbiAgICAgIHRoaXMubmV4dCA9IDA7XG4gICAgICAvLyBSZXNldHRpbmcgY29udGV4dC5fc2VudCBmb3IgbGVnYWN5IHN1cHBvcnQgb2YgQmFiZWwnc1xuICAgICAgLy8gZnVuY3Rpb24uc2VudCBpbXBsZW1lbnRhdGlvbi5cbiAgICAgIHRoaXMuc2VudCA9IHRoaXMuX3NlbnQgPSB1bmRlZmluZWQ7XG4gICAgICB0aGlzLmRvbmUgPSBmYWxzZTtcbiAgICAgIHRoaXMuZGVsZWdhdGUgPSBudWxsO1xuXG4gICAgICB0aGlzLm1ldGhvZCA9IFwibmV4dFwiO1xuICAgICAgdGhpcy5hcmcgPSB1bmRlZmluZWQ7XG5cbiAgICAgIHRoaXMudHJ5RW50cmllcy5mb3JFYWNoKHJlc2V0VHJ5RW50cnkpO1xuXG4gICAgICBpZiAoIXNraXBUZW1wUmVzZXQpIHtcbiAgICAgICAgZm9yICh2YXIgbmFtZSBpbiB0aGlzKSB7XG4gICAgICAgICAgLy8gTm90IHN1cmUgYWJvdXQgdGhlIG9wdGltYWwgb3JkZXIgb2YgdGhlc2UgY29uZGl0aW9uczpcbiAgICAgICAgICBpZiAobmFtZS5jaGFyQXQoMCkgPT09IFwidFwiICYmXG4gICAgICAgICAgICAgIGhhc093bi5jYWxsKHRoaXMsIG5hbWUpICYmXG4gICAgICAgICAgICAgICFpc05hTigrbmFtZS5zbGljZSgxKSkpIHtcbiAgICAgICAgICAgIHRoaXNbbmFtZV0gPSB1bmRlZmluZWQ7XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICB9XG4gICAgfSxcblxuICAgIHN0b3A6IGZ1bmN0aW9uKCkge1xuICAgICAgdGhpcy5kb25lID0gdHJ1ZTtcblxuICAgICAgdmFyIHJvb3RFbnRyeSA9IHRoaXMudHJ5RW50cmllc1swXTtcbiAgICAgIHZhciByb290UmVjb3JkID0gcm9vdEVudHJ5LmNvbXBsZXRpb247XG4gICAgICBpZiAocm9vdFJlY29yZC50eXBlID09PSBcInRocm93XCIpIHtcbiAgICAgICAgdGhyb3cgcm9vdFJlY29yZC5hcmc7XG4gICAgICB9XG5cbiAgICAgIHJldHVybiB0aGlzLnJ2YWw7XG4gICAgfSxcblxuICAgIGRpc3BhdGNoRXhjZXB0aW9uOiBmdW5jdGlvbihleGNlcHRpb24pIHtcbiAgICAgIGlmICh0aGlzLmRvbmUpIHtcbiAgICAgICAgdGhyb3cgZXhjZXB0aW9uO1xuICAgICAgfVxuXG4gICAgICB2YXIgY29udGV4dCA9IHRoaXM7XG4gICAgICBmdW5jdGlvbiBoYW5kbGUobG9jLCBjYXVnaHQpIHtcbiAgICAgICAgcmVjb3JkLnR5cGUgPSBcInRocm93XCI7XG4gICAgICAgIHJlY29yZC5hcmcgPSBleGNlcHRpb247XG4gICAgICAgIGNvbnRleHQubmV4dCA9IGxvYztcblxuICAgICAgICBpZiAoY2F1Z2h0KSB7XG4gICAgICAgICAgLy8gSWYgdGhlIGRpc3BhdGNoZWQgZXhjZXB0aW9uIHdhcyBjYXVnaHQgYnkgYSBjYXRjaCBibG9jayxcbiAgICAgICAgICAvLyB0aGVuIGxldCB0aGF0IGNhdGNoIGJsb2NrIGhhbmRsZSB0aGUgZXhjZXB0aW9uIG5vcm1hbGx5LlxuICAgICAgICAgIGNvbnRleHQubWV0aG9kID0gXCJuZXh0XCI7XG4gICAgICAgICAgY29udGV4dC5hcmcgPSB1bmRlZmluZWQ7XG4gICAgICAgIH1cblxuICAgICAgICByZXR1cm4gISEgY2F1Z2h0O1xuICAgICAgfVxuXG4gICAgICBmb3IgKHZhciBpID0gdGhpcy50cnlFbnRyaWVzLmxlbmd0aCAtIDE7IGkgPj0gMDsgLS1pKSB7XG4gICAgICAgIHZhciBlbnRyeSA9IHRoaXMudHJ5RW50cmllc1tpXTtcbiAgICAgICAgdmFyIHJlY29yZCA9IGVudHJ5LmNvbXBsZXRpb247XG5cbiAgICAgICAgaWYgKGVudHJ5LnRyeUxvYyA9PT0gXCJyb290XCIpIHtcbiAgICAgICAgICAvLyBFeGNlcHRpb24gdGhyb3duIG91dHNpZGUgb2YgYW55IHRyeSBibG9jayB0aGF0IGNvdWxkIGhhbmRsZVxuICAgICAgICAgIC8vIGl0LCBzbyBzZXQgdGhlIGNvbXBsZXRpb24gdmFsdWUgb2YgdGhlIGVudGlyZSBmdW5jdGlvbiB0b1xuICAgICAgICAgIC8vIHRocm93IHRoZSBleGNlcHRpb24uXG4gICAgICAgICAgcmV0dXJuIGhhbmRsZShcImVuZFwiKTtcbiAgICAgICAgfVxuXG4gICAgICAgIGlmIChlbnRyeS50cnlMb2MgPD0gdGhpcy5wcmV2KSB7XG4gICAgICAgICAgdmFyIGhhc0NhdGNoID0gaGFzT3duLmNhbGwoZW50cnksIFwiY2F0Y2hMb2NcIik7XG4gICAgICAgICAgdmFyIGhhc0ZpbmFsbHkgPSBoYXNPd24uY2FsbChlbnRyeSwgXCJmaW5hbGx5TG9jXCIpO1xuXG4gICAgICAgICAgaWYgKGhhc0NhdGNoICYmIGhhc0ZpbmFsbHkpIHtcbiAgICAgICAgICAgIGlmICh0aGlzLnByZXYgPCBlbnRyeS5jYXRjaExvYykge1xuICAgICAgICAgICAgICByZXR1cm4gaGFuZGxlKGVudHJ5LmNhdGNoTG9jLCB0cnVlKTtcbiAgICAgICAgICAgIH0gZWxzZSBpZiAodGhpcy5wcmV2IDwgZW50cnkuZmluYWxseUxvYykge1xuICAgICAgICAgICAgICByZXR1cm4gaGFuZGxlKGVudHJ5LmZpbmFsbHlMb2MpO1xuICAgICAgICAgICAgfVxuXG4gICAgICAgICAgfSBlbHNlIGlmIChoYXNDYXRjaCkge1xuICAgICAgICAgICAgaWYgKHRoaXMucHJldiA8IGVudHJ5LmNhdGNoTG9jKSB7XG4gICAgICAgICAgICAgIHJldHVybiBoYW5kbGUoZW50cnkuY2F0Y2hMb2MsIHRydWUpO1xuICAgICAgICAgICAgfVxuXG4gICAgICAgICAgfSBlbHNlIGlmIChoYXNGaW5hbGx5KSB7XG4gICAgICAgICAgICBpZiAodGhpcy5wcmV2IDwgZW50cnkuZmluYWxseUxvYykge1xuICAgICAgICAgICAgICByZXR1cm4gaGFuZGxlKGVudHJ5LmZpbmFsbHlMb2MpO1xuICAgICAgICAgICAgfVxuXG4gICAgICAgICAgfSBlbHNlIHtcbiAgICAgICAgICAgIHRocm93IG5ldyBFcnJvcihcInRyeSBzdGF0ZW1lbnQgd2l0aG91dCBjYXRjaCBvciBmaW5hbGx5XCIpO1xuICAgICAgICAgIH1cbiAgICAgICAgfVxuICAgICAgfVxuICAgIH0sXG5cbiAgICBhYnJ1cHQ6IGZ1bmN0aW9uKHR5cGUsIGFyZykge1xuICAgICAgZm9yICh2YXIgaSA9IHRoaXMudHJ5RW50cmllcy5sZW5ndGggLSAxOyBpID49IDA7IC0taSkge1xuICAgICAgICB2YXIgZW50cnkgPSB0aGlzLnRyeUVudHJpZXNbaV07XG4gICAgICAgIGlmIChlbnRyeS50cnlMb2MgPD0gdGhpcy5wcmV2ICYmXG4gICAgICAgICAgICBoYXNPd24uY2FsbChlbnRyeSwgXCJmaW5hbGx5TG9jXCIpICYmXG4gICAgICAgICAgICB0aGlzLnByZXYgPCBlbnRyeS5maW5hbGx5TG9jKSB7XG4gICAgICAgICAgdmFyIGZpbmFsbHlFbnRyeSA9IGVudHJ5O1xuICAgICAgICAgIGJyZWFrO1xuICAgICAgICB9XG4gICAgICB9XG5cbiAgICAgIGlmIChmaW5hbGx5RW50cnkgJiZcbiAgICAgICAgICAodHlwZSA9PT0gXCJicmVha1wiIHx8XG4gICAgICAgICAgIHR5cGUgPT09IFwiY29udGludWVcIikgJiZcbiAgICAgICAgICBmaW5hbGx5RW50cnkudHJ5TG9jIDw9IGFyZyAmJlxuICAgICAgICAgIGFyZyA8PSBmaW5hbGx5RW50cnkuZmluYWxseUxvYykge1xuICAgICAgICAvLyBJZ25vcmUgdGhlIGZpbmFsbHkgZW50cnkgaWYgY29udHJvbCBpcyBub3QganVtcGluZyB0byBhXG4gICAgICAgIC8vIGxvY2F0aW9uIG91dHNpZGUgdGhlIHRyeS9jYXRjaCBibG9jay5cbiAgICAgICAgZmluYWxseUVudHJ5ID0gbnVsbDtcbiAgICAgIH1cblxuICAgICAgdmFyIHJlY29yZCA9IGZpbmFsbHlFbnRyeSA/IGZpbmFsbHlFbnRyeS5jb21wbGV0aW9uIDoge307XG4gICAgICByZWNvcmQudHlwZSA9IHR5cGU7XG4gICAgICByZWNvcmQuYXJnID0gYXJnO1xuXG4gICAgICBpZiAoZmluYWxseUVudHJ5KSB7XG4gICAgICAgIHRoaXMubWV0aG9kID0gXCJuZXh0XCI7XG4gICAgICAgIHRoaXMubmV4dCA9IGZpbmFsbHlFbnRyeS5maW5hbGx5TG9jO1xuICAgICAgICByZXR1cm4gQ29udGludWVTZW50aW5lbDtcbiAgICAgIH1cblxuICAgICAgcmV0dXJuIHRoaXMuY29tcGxldGUocmVjb3JkKTtcbiAgICB9LFxuXG4gICAgY29tcGxldGU6IGZ1bmN0aW9uKHJlY29yZCwgYWZ0ZXJMb2MpIHtcbiAgICAgIGlmIChyZWNvcmQudHlwZSA9PT0gXCJ0aHJvd1wiKSB7XG4gICAgICAgIHRocm93IHJlY29yZC5hcmc7XG4gICAgICB9XG5cbiAgICAgIGlmIChyZWNvcmQudHlwZSA9PT0gXCJicmVha1wiIHx8XG4gICAgICAgICAgcmVjb3JkLnR5cGUgPT09IFwiY29udGludWVcIikge1xuICAgICAgICB0aGlzLm5leHQgPSByZWNvcmQuYXJnO1xuICAgICAgfSBlbHNlIGlmIChyZWNvcmQudHlwZSA9PT0gXCJyZXR1cm5cIikge1xuICAgICAgICB0aGlzLnJ2YWwgPSB0aGlzLmFyZyA9IHJlY29yZC5hcmc7XG4gICAgICAgIHRoaXMubWV0aG9kID0gXCJyZXR1cm5cIjtcbiAgICAgICAgdGhpcy5uZXh0ID0gXCJlbmRcIjtcbiAgICAgIH0gZWxzZSBpZiAocmVjb3JkLnR5cGUgPT09IFwibm9ybWFsXCIgJiYgYWZ0ZXJMb2MpIHtcbiAgICAgICAgdGhpcy5uZXh0ID0gYWZ0ZXJMb2M7XG4gICAgICB9XG5cbiAgICAgIHJldHVybiBDb250aW51ZVNlbnRpbmVsO1xuICAgIH0sXG5cbiAgICBmaW5pc2g6IGZ1bmN0aW9uKGZpbmFsbHlMb2MpIHtcbiAgICAgIGZvciAodmFyIGkgPSB0aGlzLnRyeUVudHJpZXMubGVuZ3RoIC0gMTsgaSA+PSAwOyAtLWkpIHtcbiAgICAgICAgdmFyIGVudHJ5ID0gdGhpcy50cnlFbnRyaWVzW2ldO1xuICAgICAgICBpZiAoZW50cnkuZmluYWxseUxvYyA9PT0gZmluYWxseUxvYykge1xuICAgICAgICAgIHRoaXMuY29tcGxldGUoZW50cnkuY29tcGxldGlvbiwgZW50cnkuYWZ0ZXJMb2MpO1xuICAgICAgICAgIHJlc2V0VHJ5RW50cnkoZW50cnkpO1xuICAgICAgICAgIHJldHVybiBDb250aW51ZVNlbnRpbmVsO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgfSxcblxuICAgIFwiY2F0Y2hcIjogZnVuY3Rpb24odHJ5TG9jKSB7XG4gICAgICBmb3IgKHZhciBpID0gdGhpcy50cnlFbnRyaWVzLmxlbmd0aCAtIDE7IGkgPj0gMDsgLS1pKSB7XG4gICAgICAgIHZhciBlbnRyeSA9IHRoaXMudHJ5RW50cmllc1tpXTtcbiAgICAgICAgaWYgKGVudHJ5LnRyeUxvYyA9PT0gdHJ5TG9jKSB7XG4gICAgICAgICAgdmFyIHJlY29yZCA9IGVudHJ5LmNvbXBsZXRpb247XG4gICAgICAgICAgaWYgKHJlY29yZC50eXBlID09PSBcInRocm93XCIpIHtcbiAgICAgICAgICAgIHZhciB0aHJvd24gPSByZWNvcmQuYXJnO1xuICAgICAgICAgICAgcmVzZXRUcnlFbnRyeShlbnRyeSk7XG4gICAgICAgICAgfVxuICAgICAgICAgIHJldHVybiB0aHJvd247XG4gICAgICAgIH1cbiAgICAgIH1cblxuICAgICAgLy8gVGhlIGNvbnRleHQuY2F0Y2ggbWV0aG9kIG11c3Qgb25seSBiZSBjYWxsZWQgd2l0aCBhIGxvY2F0aW9uXG4gICAgICAvLyBhcmd1bWVudCB0aGF0IGNvcnJlc3BvbmRzIHRvIGEga25vd24gY2F0Y2ggYmxvY2suXG4gICAgICB0aHJvdyBuZXcgRXJyb3IoXCJpbGxlZ2FsIGNhdGNoIGF0dGVtcHRcIik7XG4gICAgfSxcblxuICAgIGRlbGVnYXRlWWllbGQ6IGZ1bmN0aW9uKGl0ZXJhYmxlLCByZXN1bHROYW1lLCBuZXh0TG9jKSB7XG4gICAgICB0aGlzLmRlbGVnYXRlID0ge1xuICAgICAgICBpdGVyYXRvcjogdmFsdWVzKGl0ZXJhYmxlKSxcbiAgICAgICAgcmVzdWx0TmFtZTogcmVzdWx0TmFtZSxcbiAgICAgICAgbmV4dExvYzogbmV4dExvY1xuICAgICAgfTtcblxuICAgICAgaWYgKHRoaXMubWV0aG9kID09PSBcIm5leHRcIikge1xuICAgICAgICAvLyBEZWxpYmVyYXRlbHkgZm9yZ2V0IHRoZSBsYXN0IHNlbnQgdmFsdWUgc28gdGhhdCB3ZSBkb24ndFxuICAgICAgICAvLyBhY2NpZGVudGFsbHkgcGFzcyBpdCBvbiB0byB0aGUgZGVsZWdhdGUuXG4gICAgICAgIHRoaXMuYXJnID0gdW5kZWZpbmVkO1xuICAgICAgfVxuXG4gICAgICByZXR1cm4gQ29udGludWVTZW50aW5lbDtcbiAgICB9XG4gIH07XG5cbiAgLy8gUmVnYXJkbGVzcyBvZiB3aGV0aGVyIHRoaXMgc2NyaXB0IGlzIGV4ZWN1dGluZyBhcyBhIENvbW1vbkpTIG1vZHVsZVxuICAvLyBvciBub3QsIHJldHVybiB0aGUgcnVudGltZSBvYmplY3Qgc28gdGhhdCB3ZSBjYW4gZGVjbGFyZSB0aGUgdmFyaWFibGVcbiAgLy8gcmVnZW5lcmF0b3JSdW50aW1lIGluIHRoZSBvdXRlciBzY29wZSwgd2hpY2ggYWxsb3dzIHRoaXMgbW9kdWxlIHRvIGJlXG4gIC8vIGluamVjdGVkIGVhc2lseSBieSBgYmluL3JlZ2VuZXJhdG9yIC0taW5jbHVkZS1ydW50aW1lIHNjcmlwdC5qc2AuXG4gIHJldHVybiBleHBvcnRzO1xuXG59KFxuICAvLyBJZiB0aGlzIHNjcmlwdCBpcyBleGVjdXRpbmcgYXMgYSBDb21tb25KUyBtb2R1bGUsIHVzZSBtb2R1bGUuZXhwb3J0c1xuICAvLyBhcyB0aGUgcmVnZW5lcmF0b3JSdW50aW1lIG5hbWVzcGFjZS4gT3RoZXJ3aXNlIGNyZWF0ZSBhIG5ldyBlbXB0eVxuICAvLyBvYmplY3QuIEVpdGhlciB3YXksIHRoZSByZXN1bHRpbmcgb2JqZWN0IHdpbGwgYmUgdXNlZCB0byBpbml0aWFsaXplXG4gIC8vIHRoZSByZWdlbmVyYXRvclJ1bnRpbWUgdmFyaWFibGUgYXQgdGhlIHRvcCBvZiB0aGlzIGZpbGUuXG4gIHR5cGVvZiBtb2R1bGUgPT09IFwib2JqZWN0XCIgPyBtb2R1bGUuZXhwb3J0cyA6IHt9XG4pKTtcblxudHJ5IHtcbiAgcmVnZW5lcmF0b3JSdW50aW1lID0gcnVudGltZTtcbn0gY2F0Y2ggKGFjY2lkZW50YWxTdHJpY3RNb2RlKSB7XG4gIC8vIFRoaXMgbW9kdWxlIHNob3VsZCBub3QgYmUgcnVubmluZyBpbiBzdHJpY3QgbW9kZSwgc28gdGhlIGFib3ZlXG4gIC8vIGFzc2lnbm1lbnQgc2hvdWxkIGFsd2F5cyB3b3JrIHVubGVzcyBzb21ldGhpbmcgaXMgbWlzY29uZmlndXJlZC4gSnVzdFxuICAvLyBpbiBjYXNlIHJ1bnRpbWUuanMgYWNjaWRlbnRhbGx5IHJ1bnMgaW4gc3RyaWN0IG1vZGUsIHdlIGNhbiBlc2NhcGVcbiAgLy8gc3RyaWN0IG1vZGUgdXNpbmcgYSBnbG9iYWwgRnVuY3Rpb24gY2FsbC4gVGhpcyBjb3VsZCBjb25jZWl2YWJseSBmYWlsXG4gIC8vIGlmIGEgQ29udGVudCBTZWN1cml0eSBQb2xpY3kgZm9yYmlkcyB1c2luZyBGdW5jdGlvbiwgYnV0IGluIHRoYXQgY2FzZVxuICAvLyB0aGUgcHJvcGVyIHNvbHV0aW9uIGlzIHRvIGZpeCB0aGUgYWNjaWRlbnRhbCBzdHJpY3QgbW9kZSBwcm9ibGVtLiBJZlxuICAvLyB5b3UndmUgbWlzY29uZmlndXJlZCB5b3VyIGJ1bmRsZXIgdG8gZm9yY2Ugc3RyaWN0IG1vZGUgYW5kIGFwcGxpZWQgYVxuICAvLyBDU1AgdG8gZm9yYmlkIEZ1bmN0aW9uLCBhbmQgeW91J3JlIG5vdCB3aWxsaW5nIHRvIGZpeCBlaXRoZXIgb2YgdGhvc2VcbiAgLy8gcHJvYmxlbXMsIHBsZWFzZSBkZXRhaWwgeW91ciB1bmlxdWUgcHJlZGljYW1lbnQgaW4gYSBHaXRIdWIgaXNzdWUuXG4gIEZ1bmN0aW9uKFwiclwiLCBcInJlZ2VuZXJhdG9yUnVudGltZSA9IHJcIikocnVudGltZSk7XG59XG4iLCIvLyBNSVQgTGljZW5zZVxuLy9cbi8vIENvcHlyaWdodCAoYykgMjAxMiBVbml2ZXJzaWRhZCBQb2xpdMOpY25pY2EgZGUgTWFkcmlkXG4vL1xuLy8gUGVybWlzc2lvbiBpcyBoZXJlYnkgZ3JhbnRlZCwgZnJlZSBvZiBjaGFyZ2UsIHRvIGFueSBwZXJzb24gb2J0YWluaW5nIGEgY29weVxuLy8gb2YgdGhpcyBzb2Z0d2FyZSBhbmQgYXNzb2NpYXRlZCBkb2N1bWVudGF0aW9uIGZpbGVzICh0aGUgXCJTb2Z0d2FyZVwiKSwgdG8gZGVhbFxuLy8gaW4gdGhlIFNvZnR3YXJlIHdpdGhvdXQgcmVzdHJpY3Rpb24sIGluY2x1ZGluZyB3aXRob3V0IGxpbWl0YXRpb24gdGhlIHJpZ2h0c1xuLy8gdG8gdXNlLCBjb3B5LCBtb2RpZnksIG1lcmdlLCBwdWJsaXNoLCBkaXN0cmlidXRlLCBzdWJsaWNlbnNlLCBhbmQvb3Igc2VsbFxuLy8gY29waWVzIG9mIHRoZSBTb2Z0d2FyZSwgYW5kIHRvIHBlcm1pdCBwZXJzb25zIHRvIHdob20gdGhlIFNvZnR3YXJlIGlzXG4vLyBmdXJuaXNoZWQgdG8gZG8gc28sIHN1YmplY3QgdG8gdGhlIGZvbGxvd2luZyBjb25kaXRpb25zOlxuLy9cbi8vIFRoZSBhYm92ZSBjb3B5cmlnaHQgbm90aWNlIGFuZCB0aGlzIHBlcm1pc3Npb24gbm90aWNlIHNoYWxsIGJlIGluY2x1ZGVkIGluIGFsbFxuLy8gY29waWVzIG9yIHN1YnN0YW50aWFsIHBvcnRpb25zIG9mIHRoZSBTb2Z0d2FyZS5cbi8vXG4vLyBUSEUgU09GVFdBUkUgSVMgUFJPVklERUQgXCJBUyBJU1wiLCBXSVRIT1VUIFdBUlJBTlRZIE9GIEFOWSBLSU5ELCBFWFBSRVNTIE9SXG4vLyBJTVBMSUVELCBJTkNMVURJTkcgQlVUIE5PVCBMSU1JVEVEIFRPIFRIRSBXQVJSQU5USUVTIE9GIE1FUkNIQU5UQUJJTElUWSxcbi8vIEZJVE5FU1MgRk9SIEEgUEFSVElDVUxBUiBQVVJQT1NFIEFORCBOT05JTkZSSU5HRU1FTlQuIElOIE5PIEVWRU5UIFNIQUxMIFRIRVxuLy8gQVVUSE9SUyBPUiBDT1BZUklHSFQgSE9MREVSUyBCRSBMSUFCTEUgRk9SIEFOWSBDTEFJTSwgREFNQUdFUyBPUiBPVEhFUlxuLy8gTElBQklMSVRZLCBXSEVUSEVSIElOIEFOIEFDVElPTiBPRiBDT05UUkFDVCwgVE9SVCBPUiBPVEhFUldJU0UsIEFSSVNJTkcgRlJPTSxcbi8vIE9VVCBPRiBPUiBJTiBDT05ORUNUSU9OIFdJVEggVEhFIFNPRlRXQVJFIE9SIFRIRSBVU0UgT1IgT1RIRVIgREVBTElOR1MgSU4gVEhFXG4vLyBTT0ZUV0FSRS5cblxuLy8gQ29weXJpZ2h0IChDKSA8MjAxOD4gSW50ZWwgQ29ycG9yYXRpb25cbi8vXG4vLyBTUERYLUxpY2Vuc2UtSWRlbnRpZmllcjogQXBhY2hlLTIuMFxuXG4vLyBUaGlzIGZpbGUgaXMgYm9ycm93ZWQgZnJvbSBseW5ja2lhL2xpY29kZSB3aXRoIHNvbWUgbW9kaWZpY2F0aW9ucy5cblxuJ3VzZSBzdHJpY3QnO1xuZXhwb3J0IGNvbnN0IEJhc2U2NCA9IChmdW5jdGlvbigpIHtcbiAgY29uc3QgRU5EX09GX0lOUFVUID0gLTE7XG4gIGxldCBiYXNlNjRTdHI7XG4gIGxldCBiYXNlNjRDb3VudDtcblxuICBsZXQgaTtcblxuICBjb25zdCBiYXNlNjRDaGFycyA9IFtcbiAgICAnQScsICdCJywgJ0MnLCAnRCcsICdFJywgJ0YnLCAnRycsICdIJyxcbiAgICAnSScsICdKJywgJ0snLCAnTCcsICdNJywgJ04nLCAnTycsICdQJyxcbiAgICAnUScsICdSJywgJ1MnLCAnVCcsICdVJywgJ1YnLCAnVycsICdYJyxcbiAgICAnWScsICdaJywgJ2EnLCAnYicsICdjJywgJ2QnLCAnZScsICdmJyxcbiAgICAnZycsICdoJywgJ2knLCAnaicsICdrJywgJ2wnLCAnbScsICduJyxcbiAgICAnbycsICdwJywgJ3EnLCAncicsICdzJywgJ3QnLCAndScsICd2JyxcbiAgICAndycsICd4JywgJ3knLCAneicsICcwJywgJzEnLCAnMicsICczJyxcbiAgICAnNCcsICc1JywgJzYnLCAnNycsICc4JywgJzknLCAnKycsICcvJyxcbiAgXTtcblxuICBjb25zdCByZXZlcnNlQmFzZTY0Q2hhcnMgPSBbXTtcblxuICBmb3IgKGkgPSAwOyBpIDwgYmFzZTY0Q2hhcnMubGVuZ3RoOyBpID0gaSArIDEpIHtcbiAgICByZXZlcnNlQmFzZTY0Q2hhcnNbYmFzZTY0Q2hhcnNbaV1dID0gaTtcbiAgfVxuXG4gIGNvbnN0IHNldEJhc2U2NFN0ciA9IGZ1bmN0aW9uKHN0cikge1xuICAgIGJhc2U2NFN0ciA9IHN0cjtcbiAgICBiYXNlNjRDb3VudCA9IDA7XG4gIH07XG5cbiAgY29uc3QgcmVhZEJhc2U2NCA9IGZ1bmN0aW9uKCkge1xuICAgIGlmICghYmFzZTY0U3RyKSB7XG4gICAgICByZXR1cm4gRU5EX09GX0lOUFVUO1xuICAgIH1cbiAgICBpZiAoYmFzZTY0Q291bnQgPj0gYmFzZTY0U3RyLmxlbmd0aCkge1xuICAgICAgcmV0dXJuIEVORF9PRl9JTlBVVDtcbiAgICB9XG4gICAgY29uc3QgYyA9IGJhc2U2NFN0ci5jaGFyQ29kZUF0KGJhc2U2NENvdW50KSAmIDB4ZmY7XG4gICAgYmFzZTY0Q291bnQgPSBiYXNlNjRDb3VudCArIDE7XG4gICAgcmV0dXJuIGM7XG4gIH07XG5cbiAgY29uc3QgZW5jb2RlQmFzZTY0ID0gZnVuY3Rpb24oc3RyKSB7XG4gICAgbGV0IHJlc3VsdDtcbiAgICBsZXQgZG9uZTtcbiAgICBzZXRCYXNlNjRTdHIoc3RyKTtcbiAgICByZXN1bHQgPSAnJztcbiAgICBjb25zdCBpbkJ1ZmZlciA9IG5ldyBBcnJheSgzKTtcbiAgICBkb25lID0gZmFsc2U7XG4gICAgd2hpbGUgKCFkb25lICYmIChpbkJ1ZmZlclswXSA9IHJlYWRCYXNlNjQoKSkgIT09IEVORF9PRl9JTlBVVCkge1xuICAgICAgaW5CdWZmZXJbMV0gPSByZWFkQmFzZTY0KCk7XG4gICAgICBpbkJ1ZmZlclsyXSA9IHJlYWRCYXNlNjQoKTtcbiAgICAgIHJlc3VsdCA9IHJlc3VsdCArIChiYXNlNjRDaGFyc1tpbkJ1ZmZlclswXSA+PiAyXSk7XG4gICAgICBpZiAoaW5CdWZmZXJbMV0gIT09IEVORF9PRl9JTlBVVCkge1xuICAgICAgICByZXN1bHQgPSByZXN1bHQgKyAoYmFzZTY0Q2hhcnNbKChpbkJ1ZmZlclswXSA8PCA0KSAmIDB4MzApIHwgKFxuICAgICAgICAgIGluQnVmZmVyWzFdID4+IDQpXSk7XG4gICAgICAgIGlmIChpbkJ1ZmZlclsyXSAhPT0gRU5EX09GX0lOUFVUKSB7XG4gICAgICAgICAgcmVzdWx0ID0gcmVzdWx0ICsgKGJhc2U2NENoYXJzWygoaW5CdWZmZXJbMV0gPDwgMikgJiAweDNjKSB8IChcbiAgICAgICAgICAgIGluQnVmZmVyWzJdID4+IDYpXSk7XG4gICAgICAgICAgcmVzdWx0ID0gcmVzdWx0ICsgKGJhc2U2NENoYXJzW2luQnVmZmVyWzJdICYgMHgzRl0pO1xuICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgIHJlc3VsdCA9IHJlc3VsdCArIChiYXNlNjRDaGFyc1soKGluQnVmZmVyWzFdIDw8IDIpICYgMHgzYyldKTtcbiAgICAgICAgICByZXN1bHQgPSByZXN1bHQgKyAoJz0nKTtcbiAgICAgICAgICBkb25lID0gdHJ1ZTtcbiAgICAgICAgfVxuICAgICAgfSBlbHNlIHtcbiAgICAgICAgcmVzdWx0ID0gcmVzdWx0ICsgKGJhc2U2NENoYXJzWygoaW5CdWZmZXJbMF0gPDwgNCkgJiAweDMwKV0pO1xuICAgICAgICByZXN1bHQgPSByZXN1bHQgKyAoJz0nKTtcbiAgICAgICAgcmVzdWx0ID0gcmVzdWx0ICsgKCc9Jyk7XG4gICAgICAgIGRvbmUgPSB0cnVlO1xuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gcmVzdWx0O1xuICB9O1xuXG4gIGNvbnN0IHJlYWRSZXZlcnNlQmFzZTY0ID0gZnVuY3Rpb24oKSB7XG4gICAgaWYgKCFiYXNlNjRTdHIpIHtcbiAgICAgIHJldHVybiBFTkRfT0ZfSU5QVVQ7XG4gICAgfVxuICAgIHdoaWxlICh0cnVlKSB7IC8vIGVzbGludC1kaXNhYmxlLWxpbmUgbm8tY29uc3RhbnQtY29uZGl0aW9uXG4gICAgICBpZiAoYmFzZTY0Q291bnQgPj0gYmFzZTY0U3RyLmxlbmd0aCkge1xuICAgICAgICByZXR1cm4gRU5EX09GX0lOUFVUO1xuICAgICAgfVxuICAgICAgY29uc3QgbmV4dENoYXJhY3RlciA9IGJhc2U2NFN0ci5jaGFyQXQoYmFzZTY0Q291bnQpO1xuICAgICAgYmFzZTY0Q291bnQgPSBiYXNlNjRDb3VudCArIDE7XG4gICAgICBpZiAocmV2ZXJzZUJhc2U2NENoYXJzW25leHRDaGFyYWN0ZXJdKSB7XG4gICAgICAgIHJldHVybiByZXZlcnNlQmFzZTY0Q2hhcnNbbmV4dENoYXJhY3Rlcl07XG4gICAgICB9XG4gICAgICBpZiAobmV4dENoYXJhY3RlciA9PT0gJ0EnKSB7XG4gICAgICAgIHJldHVybiAwO1xuICAgICAgfVxuICAgIH1cbiAgfTtcblxuICBjb25zdCBudG9zID0gZnVuY3Rpb24obikge1xuICAgIG4gPSBuLnRvU3RyaW5nKDE2KTtcbiAgICBpZiAobi5sZW5ndGggPT09IDEpIHtcbiAgICAgIG4gPSAnMCcgKyBuO1xuICAgIH1cbiAgICBuID0gJyUnICsgbjtcbiAgICByZXR1cm4gdW5lc2NhcGUobik7XG4gIH07XG5cbiAgY29uc3QgZGVjb2RlQmFzZTY0ID0gZnVuY3Rpb24oc3RyKSB7XG4gICAgbGV0IHJlc3VsdDtcbiAgICBsZXQgZG9uZTtcbiAgICBzZXRCYXNlNjRTdHIoc3RyKTtcbiAgICByZXN1bHQgPSAnJztcbiAgICBjb25zdCBpbkJ1ZmZlciA9IG5ldyBBcnJheSg0KTtcbiAgICBkb25lID0gZmFsc2U7XG4gICAgd2hpbGUgKCFkb25lICYmIChpbkJ1ZmZlclswXSA9IHJlYWRSZXZlcnNlQmFzZTY0KCkpICE9PSBFTkRfT0ZfSU5QVVQgJiZcbiAgICAgIChpbkJ1ZmZlclsxXSA9IHJlYWRSZXZlcnNlQmFzZTY0KCkpICE9PSBFTkRfT0ZfSU5QVVQpIHtcbiAgICAgIGluQnVmZmVyWzJdID0gcmVhZFJldmVyc2VCYXNlNjQoKTtcbiAgICAgIGluQnVmZmVyWzNdID0gcmVhZFJldmVyc2VCYXNlNjQoKTtcbiAgICAgIHJlc3VsdCA9IHJlc3VsdCArIG50b3MoKCgoaW5CdWZmZXJbMF0gPDwgMikgJiAweGZmKSB8IGluQnVmZmVyWzFdID4+XG4gICAgICAgIDQpKTtcbiAgICAgIGlmIChpbkJ1ZmZlclsyXSAhPT0gRU5EX09GX0lOUFVUKSB7XG4gICAgICAgIHJlc3VsdCArPSBudG9zKCgoKGluQnVmZmVyWzFdIDw8IDQpICYgMHhmZikgfCBpbkJ1ZmZlclsyXSA+PiAyKSk7XG4gICAgICAgIGlmIChpbkJ1ZmZlclszXSAhPT0gRU5EX09GX0lOUFVUKSB7XG4gICAgICAgICAgcmVzdWx0ID0gcmVzdWx0ICsgbnRvcygoKChpbkJ1ZmZlclsyXSA8PCA2KSAmIDB4ZmYpIHwgaW5CdWZmZXJbXG4gICAgICAgICAgICAgIDNdKSk7XG4gICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgZG9uZSA9IHRydWU7XG4gICAgICAgIH1cbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIGRvbmUgPSB0cnVlO1xuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gcmVzdWx0O1xuICB9O1xuXG4gIHJldHVybiB7XG4gICAgZW5jb2RlQmFzZTY0OiBlbmNvZGVCYXNlNjQsXG4gICAgZGVjb2RlQmFzZTY0OiBkZWNvZGVCYXNlNjQsXG4gIH07XG59KCkpO1xuIiwiLy8gQ29weXJpZ2h0IChDKSA8MjAxOD4gSW50ZWwgQ29ycG9yYXRpb25cbi8vXG4vLyBTUERYLUxpY2Vuc2UtSWRlbnRpZmllcjogQXBhY2hlLTIuMFxuXG4ndXNlIHN0cmljdCc7XG5cbi8qKlxuICogQGNsYXNzIEF1ZGlvQ29kZWNcbiAqIEBtZW1iZXJPZiBPd3QuQmFzZVxuICogQGNsYXNzRGVzYyBBdWRpbyBjb2RlYyBlbnVtZXJhdGlvbi5cbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNvbnN0IEF1ZGlvQ29kZWMgPSB7XG4gIFBDTVU6ICdwY211JyxcbiAgUENNQTogJ3BjbWEnLFxuICBPUFVTOiAnb3B1cycsXG4gIEc3MjI6ICdnNzIyJyxcbiAgSVNBQzogJ2lTQUMnLFxuICBJTEJDOiAnaUxCQycsXG4gIEFBQzogJ2FhYycsXG4gIEFDMzogJ2FjMycsXG4gIE5FTExZTU9TRVI6ICduZWxseW1vc2VyJyxcbn07XG4vKipcbiAqIEBjbGFzcyBBdWRpb0NvZGVjUGFyYW1ldGVyc1xuICogQG1lbWJlck9mIE93dC5CYXNlXG4gKiBAY2xhc3NEZXNjIENvZGVjIHBhcmFtZXRlcnMgZm9yIGFuIGF1ZGlvIHRyYWNrLlxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY2xhc3MgQXVkaW9Db2RlY1BhcmFtZXRlcnMge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3RvcihuYW1lLCBjaGFubmVsQ291bnQsIGNsb2NrUmF0ZSkge1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge3N0cmluZ30gbmFtZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5BdWRpb0NvZGVjUGFyYW1ldGVyc1xuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBkZXNjIE5hbWUgb2YgYSBjb2RlYy4gUGxlYXNlIGEgdmFsdWUgaW4gT3d0LkJhc2UuQXVkaW9Db2RlYy4gSG93ZXZlcixcbiAgICAgKiBzb21lIGZ1bmN0aW9ucyBkbyBub3Qgc3VwcG9ydCBhbGwgdGhlIHZhbHVlcyBpbiBPd3QuQmFzZS5BdWRpb0NvZGVjLlxuICAgICAqL1xuICAgIHRoaXMubmFtZSA9IG5hbWU7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P251bWJlcn0gY2hhbm5lbENvdW50XG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLkF1ZGlvQ29kZWNQYXJhbWV0ZXJzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQGRlc2MgTnVtYmVycyBvZiBjaGFubmVscyBmb3IgYW4gYXVkaW8gdHJhY2suXG4gICAgICovXG4gICAgdGhpcy5jaGFubmVsQ291bnQgPSBjaGFubmVsQ291bnQ7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P251bWJlcn0gY2xvY2tSYXRlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLkF1ZGlvQ29kZWNQYXJhbWV0ZXJzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQGRlc2MgVGhlIGNvZGVjIGNsb2NrIHJhdGUgZXhwcmVzc2VkIGluIEhlcnR6LlxuICAgICAqL1xuICAgIHRoaXMuY2xvY2tSYXRlID0gY2xvY2tSYXRlO1xuICB9XG59XG5cbi8qKlxuICogQGNsYXNzIEF1ZGlvRW5jb2RpbmdQYXJhbWV0ZXJzXG4gKiBAbWVtYmVyT2YgT3d0LkJhc2VcbiAqIEBjbGFzc0Rlc2MgRW5jb2RpbmcgcGFyYW1ldGVycyBmb3Igc2VuZGluZyBhbiBhdWRpbyB0cmFjay5cbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNsYXNzIEF1ZGlvRW5jb2RpbmdQYXJhbWV0ZXJzIHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IoY29kZWMsIG1heEJpdHJhdGUpIHtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/T3d0LkJhc2UuQXVkaW9Db2RlY1BhcmFtZXRlcnN9IGNvZGVjXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLkF1ZGlvRW5jb2RpbmdQYXJhbWV0ZXJzXG4gICAgICovXG4gICAgdGhpcy5jb2RlYyA9IGNvZGVjO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIgez9udW1iZXJ9IG1heEJpdHJhdGVcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuQXVkaW9FbmNvZGluZ1BhcmFtZXRlcnNcbiAgICAgKiBAZGVzYyBNYXggYml0cmF0ZSBleHByZXNzZWQgaW4ga2Jwcy5cbiAgICAgKi9cbiAgICB0aGlzLm1heEJpdHJhdGUgPSBtYXhCaXRyYXRlO1xuICB9XG59XG5cbi8qKlxuICogQGNsYXNzIFZpZGVvQ29kZWNcbiAqIEBtZW1iZXJPZiBPd3QuQmFzZVxuICogQGNsYXNzRGVzYyBWaWRlbyBjb2RlYyBlbnVtZXJhdGlvbi5cbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNvbnN0IFZpZGVvQ29kZWMgPSB7XG4gIFZQODogJ3ZwOCcsXG4gIFZQOTogJ3ZwOScsXG4gIEgyNjQ6ICdoMjY0JyxcbiAgSDI2NTogJ2gyNjUnLFxuICBBVjE6ICdhdjEnLFxuICAvLyBOb24tc3RhbmRhcmQgQVYxLCB3aWxsIGJlIHJlbmFtZWQgdG8gQVYxLlxuICAvLyBodHRwczovL2J1Z3MuY2hyb21pdW0ub3JnL3Avd2VicnRjL2lzc3Vlcy9kZXRhaWw/aWQ9MTEwNDJcbiAgQVYxWDogJ2F2MXgnLFxufTtcblxuLyoqXG4gKiBAY2xhc3MgVmlkZW9Db2RlY1BhcmFtZXRlcnNcbiAqIEBtZW1iZXJPZiBPd3QuQmFzZVxuICogQGNsYXNzRGVzYyBDb2RlYyBwYXJhbWV0ZXJzIGZvciBhIHZpZGVvIHRyYWNrLlxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY2xhc3MgVmlkZW9Db2RlY1BhcmFtZXRlcnMge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3RvcihuYW1lLCBwcm9maWxlKSB7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7c3RyaW5nfSBuYW1lXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlZpZGVvQ29kZWNQYXJhbWV0ZXJzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQGRlc2MgTmFtZSBvZiBhIGNvZGVjLlBsZWFzZSBhIHZhbHVlIGluIE93dC5CYXNlLkF1ZGlvQ29kZWMuSG93ZXZlcixcbiAgICAgICBzb21lIGZ1bmN0aW9ucyBkbyBub3Qgc3VwcG9ydCBhbGwgdGhlIHZhbHVlcyBpbiBPd3QuQmFzZS5BdWRpb0NvZGVjLlxuICAgICAqL1xuICAgIHRoaXMubmFtZSA9IG5hbWU7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P3N0cmluZ30gcHJvZmlsZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5WaWRlb0NvZGVjUGFyYW1ldGVyc1xuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBkZXNjIFRoZSBwcm9maWxlIG9mIGEgY29kZWMuIFByb2ZpbGUgbWF5IG5vdCBhcHBseSB0byBhbGwgY29kZWNzLlxuICAgICAqL1xuICAgIHRoaXMucHJvZmlsZSA9IHByb2ZpbGU7XG4gIH1cbn1cblxuLyoqXG4gKiBAY2xhc3MgVmlkZW9FbmNvZGluZ1BhcmFtZXRlcnNcbiAqIEBtZW1iZXJPZiBPd3QuQmFzZVxuICogQGNsYXNzRGVzYyBFbmNvZGluZyBwYXJhbWV0ZXJzIGZvciBzZW5kaW5nIGEgdmlkZW8gdHJhY2suXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKi9cbmV4cG9ydCBjbGFzcyBWaWRlb0VuY29kaW5nUGFyYW1ldGVycyB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKGNvZGVjLCBtYXhCaXRyYXRlKSB7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P093dC5CYXNlLlZpZGVvQ29kZWNQYXJhbWV0ZXJzfSBjb2RlY1xuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5WaWRlb0VuY29kaW5nUGFyYW1ldGVyc1xuICAgICAqL1xuICAgIHRoaXMuY29kZWMgPSBjb2RlYztcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/bnVtYmVyfSBtYXhCaXRyYXRlXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlZpZGVvRW5jb2RpbmdQYXJhbWV0ZXJzXG4gICAgICogQGRlc2MgTWF4IGJpdHJhdGUgZXhwcmVzc2VkIGluIGticHMuXG4gICAgICovXG4gICAgdGhpcy5tYXhCaXRyYXRlID0gbWF4Qml0cmF0ZTtcbiAgfVxufVxuIiwiLy8gTUlUIExpY2Vuc2Vcbi8vXG4vLyBDb3B5cmlnaHQgKGMpIDIwMTIgVW5pdmVyc2lkYWQgUG9saXTDqWNuaWNhIGRlIE1hZHJpZFxuLy9cbi8vIFBlcm1pc3Npb24gaXMgaGVyZWJ5IGdyYW50ZWQsIGZyZWUgb2YgY2hhcmdlLCB0byBhbnkgcGVyc29uIG9idGFpbmluZyBhIGNvcHlcbi8vIG9mIHRoaXMgc29mdHdhcmUgYW5kIGFzc29jaWF0ZWQgZG9jdW1lbnRhdGlvbiBmaWxlcyAodGhlIFwiU29mdHdhcmVcIiksIHRvIGRlYWxcbi8vIGluIHRoZSBTb2Z0d2FyZSB3aXRob3V0IHJlc3RyaWN0aW9uLCBpbmNsdWRpbmcgd2l0aG91dCBsaW1pdGF0aW9uIHRoZSByaWdodHNcbi8vIHRvIHVzZSwgY29weSwgbW9kaWZ5LCBtZXJnZSwgcHVibGlzaCwgZGlzdHJpYnV0ZSwgc3VibGljZW5zZSwgYW5kL29yIHNlbGxcbi8vIGNvcGllcyBvZiB0aGUgU29mdHdhcmUsIGFuZCB0byBwZXJtaXQgcGVyc29ucyB0byB3aG9tIHRoZSBTb2Z0d2FyZSBpc1xuLy8gZnVybmlzaGVkIHRvIGRvIHNvLCBzdWJqZWN0IHRvIHRoZSBmb2xsb3dpbmcgY29uZGl0aW9uczpcbi8vXG4vLyBUaGUgYWJvdmUgY29weXJpZ2h0IG5vdGljZSBhbmQgdGhpcyBwZXJtaXNzaW9uIG5vdGljZSBzaGFsbCBiZSBpbmNsdWRlZCBpbiBhbGxcbi8vIGNvcGllcyBvciBzdWJzdGFudGlhbCBwb3J0aW9ucyBvZiB0aGUgU29mdHdhcmUuXG4vL1xuLy8gVEhFIFNPRlRXQVJFIElTIFBST1ZJREVEIFwiQVMgSVNcIiwgV0lUSE9VVCBXQVJSQU5UWSBPRiBBTlkgS0lORCwgRVhQUkVTUyBPUlxuLy8gSU1QTElFRCwgSU5DTFVESU5HIEJVVCBOT1QgTElNSVRFRCBUTyBUSEUgV0FSUkFOVElFUyBPRiBNRVJDSEFOVEFCSUxJVFksXG4vLyBGSVRORVNTIEZPUiBBIFBBUlRJQ1VMQVIgUFVSUE9TRSBBTkQgTk9OSU5GUklOR0VNRU5ULiBJTiBOTyBFVkVOVCBTSEFMTCBUSEVcbi8vIEFVVEhPUlMgT1IgQ09QWVJJR0hUIEhPTERFUlMgQkUgTElBQkxFIEZPUiBBTlkgQ0xBSU0sIERBTUFHRVMgT1IgT1RIRVJcbi8vIExJQUJJTElUWSwgV0hFVEhFUiBJTiBBTiBBQ1RJT04gT0YgQ09OVFJBQ1QsIFRPUlQgT1IgT1RIRVJXSVNFLCBBUklTSU5HIEZST00sXG4vLyBPVVQgT0YgT1IgSU4gQ09OTkVDVElPTiBXSVRIIFRIRSBTT0ZUV0FSRSBPUiBUSEUgVVNFIE9SIE9USEVSIERFQUxJTkdTIElOIFRIRVxuLy8gU09GVFdBUkUuXG5cbi8vIENvcHlyaWdodCAoQykgPDIwMTg+IEludGVsIENvcnBvcmF0aW9uXG4vL1xuLy8gU1BEWC1MaWNlbnNlLUlkZW50aWZpZXI6IEFwYWNoZS0yLjBcblxuLy8gVGhpcyBmaWxlIGlzIGJvcnJvd2VkIGZyb20gbHluY2tpYS9saWNvZGUgd2l0aCBzb21lIG1vZGlmaWNhdGlvbnMuXG5cbid1c2Ugc3RyaWN0JztcblxuLyoqXG4gKiBAY2xhc3MgRXZlbnREaXNwYXRjaGVyXG4gKiBAY2xhc3NEZXNjIEEgc2hpbSBmb3IgRXZlbnRUYXJnZXQuIE1pZ2h0IGJlIGNoYW5nZWQgdG8gRXZlbnRUYXJnZXQgbGF0ZXIuXG4gKiBAbWVtYmVyb2YgT3d0LkJhc2VcbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNvbnN0IEV2ZW50RGlzcGF0Y2hlciA9IGZ1bmN0aW9uKCkge1xuICAvLyBQcml2YXRlIHZhcnNcbiAgY29uc3Qgc3BlYyA9IHt9O1xuICBzcGVjLmRpc3BhdGNoZXIgPSB7fTtcbiAgc3BlYy5kaXNwYXRjaGVyLmV2ZW50TGlzdGVuZXJzID0ge307XG5cbiAgLyoqXG4gICAqIEBmdW5jdGlvbiBhZGRFdmVudExpc3RlbmVyXG4gICAqIEBkZXNjIFRoaXMgZnVuY3Rpb24gcmVnaXN0ZXJzIGEgY2FsbGJhY2sgZnVuY3Rpb24gYXMgYSBoYW5kbGVyIGZvciB0aGVcbiAgICogY29ycmVzcG9uZGluZyBldmVudC4gSXQncyBzaG9ydGVuZWQgZm9ybSBpcyBvbihldmVudFR5cGUsIGxpc3RlbmVyKS4gU2VlXG4gICAqIHRoZSBldmVudCBkZXNjcmlwdGlvbiBpbiB0aGUgZm9sbG93aW5nIHRhYmxlLlxuICAgKiBAaW5zdGFuY2VcbiAgICogQG1lbWJlcm9mIE93dC5CYXNlLkV2ZW50RGlzcGF0Y2hlclxuICAgKiBAcGFyYW0ge3N0cmluZ30gZXZlbnRUeXBlIEV2ZW50IHN0cmluZy5cbiAgICogQHBhcmFtIHtmdW5jdGlvbn0gbGlzdGVuZXIgQ2FsbGJhY2sgZnVuY3Rpb24uXG4gICAqL1xuICB0aGlzLmFkZEV2ZW50TGlzdGVuZXIgPSBmdW5jdGlvbihldmVudFR5cGUsIGxpc3RlbmVyKSB7XG4gICAgaWYgKHNwZWMuZGlzcGF0Y2hlci5ldmVudExpc3RlbmVyc1tldmVudFR5cGVdID09PSB1bmRlZmluZWQpIHtcbiAgICAgIHNwZWMuZGlzcGF0Y2hlci5ldmVudExpc3RlbmVyc1tldmVudFR5cGVdID0gW107XG4gICAgfVxuICAgIHNwZWMuZGlzcGF0Y2hlci5ldmVudExpc3RlbmVyc1tldmVudFR5cGVdLnB1c2gobGlzdGVuZXIpO1xuICB9O1xuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gcmVtb3ZlRXZlbnRMaXN0ZW5lclxuICAgKiBAZGVzYyBUaGlzIGZ1bmN0aW9uIHJlbW92ZXMgYSByZWdpc3RlcmVkIGV2ZW50IGxpc3RlbmVyLlxuICAgKiBAaW5zdGFuY2VcbiAgICogQG1lbWJlcm9mIE93dC5CYXNlLkV2ZW50RGlzcGF0Y2hlclxuICAgKiBAcGFyYW0ge3N0cmluZ30gZXZlbnRUeXBlIEV2ZW50IHN0cmluZy5cbiAgICogQHBhcmFtIHtmdW5jdGlvbn0gbGlzdGVuZXIgQ2FsbGJhY2sgZnVuY3Rpb24uXG4gICAqL1xuICB0aGlzLnJlbW92ZUV2ZW50TGlzdGVuZXIgPSBmdW5jdGlvbihldmVudFR5cGUsIGxpc3RlbmVyKSB7XG4gICAgaWYgKCFzcGVjLmRpc3BhdGNoZXIuZXZlbnRMaXN0ZW5lcnNbZXZlbnRUeXBlXSkge1xuICAgICAgcmV0dXJuO1xuICAgIH1cbiAgICBjb25zdCBpbmRleCA9IHNwZWMuZGlzcGF0Y2hlci5ldmVudExpc3RlbmVyc1tldmVudFR5cGVdLmluZGV4T2YobGlzdGVuZXIpO1xuICAgIGlmIChpbmRleCAhPT0gLTEpIHtcbiAgICAgIHNwZWMuZGlzcGF0Y2hlci5ldmVudExpc3RlbmVyc1tldmVudFR5cGVdLnNwbGljZShpbmRleCwgMSk7XG4gICAgfVxuICB9O1xuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gY2xlYXJFdmVudExpc3RlbmVyXG4gICAqIEBkZXNjIFRoaXMgZnVuY3Rpb24gcmVtb3ZlcyBhbGwgZXZlbnQgbGlzdGVuZXJzIGZvciBvbmUgdHlwZS5cbiAgICogQGluc3RhbmNlXG4gICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5FdmVudERpc3BhdGNoZXJcbiAgICogQHBhcmFtIHtzdHJpbmd9IGV2ZW50VHlwZSBFdmVudCBzdHJpbmcuXG4gICAqL1xuICB0aGlzLmNsZWFyRXZlbnRMaXN0ZW5lciA9IGZ1bmN0aW9uKGV2ZW50VHlwZSkge1xuICAgIHNwZWMuZGlzcGF0Y2hlci5ldmVudExpc3RlbmVyc1tldmVudFR5cGVdID0gW107XG4gIH07XG5cbiAgLy8gSXQgZGlzcGF0Y2ggYSBuZXcgZXZlbnQgdG8gdGhlIGV2ZW50IGxpc3RlbmVycywgYmFzZWQgb24gdGhlIHR5cGVcbiAgLy8gb2YgZXZlbnQuIEFsbCBldmVudHMgYXJlIGludGVuZGVkIHRvIGJlIExpY29kZUV2ZW50cy5cbiAgdGhpcy5kaXNwYXRjaEV2ZW50ID0gZnVuY3Rpb24oZXZlbnQpIHtcbiAgICBpZiAoIXNwZWMuZGlzcGF0Y2hlci5ldmVudExpc3RlbmVyc1tldmVudC50eXBlXSkge1xuICAgICAgcmV0dXJuO1xuICAgIH1cbiAgICBzcGVjLmRpc3BhdGNoZXIuZXZlbnRMaXN0ZW5lcnNbZXZlbnQudHlwZV0ubWFwKGZ1bmN0aW9uKGxpc3RlbmVyKSB7XG4gICAgICBsaXN0ZW5lcihldmVudCk7XG4gICAgfSk7XG4gIH07XG59O1xuXG4vKipcbiAqIEBjbGFzcyBPd3RFdmVudFxuICogQGNsYXNzRGVzYyBDbGFzcyBPd3RFdmVudCByZXByZXNlbnRzIGEgZ2VuZXJpYyBFdmVudCBpbiB0aGUgbGlicmFyeS5cbiAqIEBtZW1iZXJvZiBPd3QuQmFzZVxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY2xhc3MgT3d0RXZlbnQge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3Rvcih0eXBlKSB7XG4gICAgdGhpcy50eXBlID0gdHlwZTtcbiAgfVxufVxuXG4vKipcbiAqIEBjbGFzcyBNZXNzYWdlRXZlbnRcbiAqIEBjbGFzc0Rlc2MgQ2xhc3MgTWVzc2FnZUV2ZW50IHJlcHJlc2VudHMgYSBtZXNzYWdlIEV2ZW50IGluIHRoZSBsaWJyYXJ5LlxuICogQG1lbWJlcm9mIE93dC5CYXNlXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKi9cbmV4cG9ydCBjbGFzcyBNZXNzYWdlRXZlbnQgZXh0ZW5kcyBPd3RFdmVudCB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKHR5cGUsIGluaXQpIHtcbiAgICBzdXBlcih0eXBlKTtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHtzdHJpbmd9IG9yaWdpblxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5NZXNzYWdlRXZlbnRcbiAgICAgKiBAZGVzYyBJRCBvZiB0aGUgcmVtb3RlIGVuZHBvaW50IHdobyBwdWJsaXNoZWQgdGhpcyBzdHJlYW0uXG4gICAgICovXG4gICAgdGhpcy5vcmlnaW4gPSBpbml0Lm9yaWdpbjtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHtzdHJpbmd9IG1lc3NhZ2VcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuTWVzc2FnZUV2ZW50XG4gICAgICovXG4gICAgdGhpcy5tZXNzYWdlID0gaW5pdC5tZXNzYWdlO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge3N0cmluZ30gdG9cbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuTWVzc2FnZUV2ZW50XG4gICAgICogQGRlc2MgVmFsdWVzIGNvdWxkIGJlIFwiYWxsXCIsIFwibWVcIiBpbiBjb25mZXJlbmNlIG1vZGUsIG9yIHVuZGVmaW5lZCBpblxuICAgICAqIFAyUCBtb2RlLlxuICAgICAqL1xuICAgIHRoaXMudG8gPSBpbml0LnRvO1xuICB9XG59XG5cbi8qKlxuICogQGNsYXNzIEVycm9yRXZlbnRcbiAqIEBjbGFzc0Rlc2MgQ2xhc3MgRXJyb3JFdmVudCByZXByZXNlbnRzIGFuIGVycm9yIEV2ZW50IGluIHRoZSBsaWJyYXJ5LlxuICogQG1lbWJlcm9mIE93dC5CYXNlXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKi9cbmV4cG9ydCBjbGFzcyBFcnJvckV2ZW50IGV4dGVuZHMgT3d0RXZlbnQge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3Rvcih0eXBlLCBpbml0KSB7XG4gICAgc3VwZXIodHlwZSk7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7RXJyb3J9IGVycm9yXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLkVycm9yRXZlbnRcbiAgICAgKi9cbiAgICB0aGlzLmVycm9yID0gaW5pdC5lcnJvcjtcbiAgfVxufVxuXG4vKipcbiAqIEBjbGFzcyBNdXRlRXZlbnRcbiAqIEBjbGFzc0Rlc2MgQ2xhc3MgTXV0ZUV2ZW50IHJlcHJlc2VudHMgYSBtdXRlIG9yIHVubXV0ZSBldmVudC5cbiAqIEBtZW1iZXJvZiBPd3QuQmFzZVxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY2xhc3MgTXV0ZUV2ZW50IGV4dGVuZHMgT3d0RXZlbnQge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3Rvcih0eXBlLCBpbml0KSB7XG4gICAgc3VwZXIodHlwZSk7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7T3d0LkJhc2UuVHJhY2tLaW5kfSBraW5kXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLk11dGVFdmVudFxuICAgICAqL1xuICAgIHRoaXMua2luZCA9IGluaXQua2luZDtcbiAgfVxufVxuIiwiLy8gQ29weXJpZ2h0IChDKSA8MjAxOD4gSW50ZWwgQ29ycG9yYXRpb25cbi8vXG4vLyBTUERYLUxpY2Vuc2UtSWRlbnRpZmllcjogQXBhY2hlLTIuMFxuXG4ndXNlIHN0cmljdCc7XG5cbmV4cG9ydCAqIGZyb20gJy4vbWVkaWFzdHJlYW0tZmFjdG9yeS5qcyc7XG5leHBvcnQgKiBmcm9tICcuL3N0cmVhbS5qcyc7XG5leHBvcnQgKiBmcm9tICcuL21lZGlhZm9ybWF0LmpzJztcbmV4cG9ydCAqIGZyb20gJy4vdHJhbnNwb3J0LmpzJztcbiIsIi8vIE1JVCBMaWNlbnNlXG4vL1xuLy8gQ29weXJpZ2h0IChjKSAyMDEyIFVuaXZlcnNpZGFkIFBvbGl0w6ljbmljYSBkZSBNYWRyaWRcbi8vXG4vLyBQZXJtaXNzaW9uIGlzIGhlcmVieSBncmFudGVkLCBmcmVlIG9mIGNoYXJnZSwgdG8gYW55IHBlcnNvbiBvYnRhaW5pbmcgYSBjb3B5XG4vLyBvZiB0aGlzIHNvZnR3YXJlIGFuZCBhc3NvY2lhdGVkIGRvY3VtZW50YXRpb24gZmlsZXMgKHRoZSBcIlNvZnR3YXJlXCIpLCB0byBkZWFsXG4vLyBpbiB0aGUgU29mdHdhcmUgd2l0aG91dCByZXN0cmljdGlvbiwgaW5jbHVkaW5nIHdpdGhvdXQgbGltaXRhdGlvbiB0aGUgcmlnaHRzXG4vLyB0byB1c2UsIGNvcHksIG1vZGlmeSwgbWVyZ2UsIHB1Ymxpc2gsIGRpc3RyaWJ1dGUsIHN1YmxpY2Vuc2UsIGFuZC9vciBzZWxsXG4vLyBjb3BpZXMgb2YgdGhlIFNvZnR3YXJlLCBhbmQgdG8gcGVybWl0IHBlcnNvbnMgdG8gd2hvbSB0aGUgU29mdHdhcmUgaXNcbi8vIGZ1cm5pc2hlZCB0byBkbyBzbywgc3ViamVjdCB0byB0aGUgZm9sbG93aW5nIGNvbmRpdGlvbnM6XG4vL1xuLy8gVGhlIGFib3ZlIGNvcHlyaWdodCBub3RpY2UgYW5kIHRoaXMgcGVybWlzc2lvbiBub3RpY2Ugc2hhbGwgYmUgaW5jbHVkZWQgaW4gYWxsXG4vLyBjb3BpZXMgb3Igc3Vic3RhbnRpYWwgcG9ydGlvbnMgb2YgdGhlIFNvZnR3YXJlLlxuLy9cbi8vIFRIRSBTT0ZUV0FSRSBJUyBQUk9WSURFRCBcIkFTIElTXCIsIFdJVEhPVVQgV0FSUkFOVFkgT0YgQU5ZIEtJTkQsIEVYUFJFU1MgT1Jcbi8vIElNUExJRUQsIElOQ0xVRElORyBCVVQgTk9UIExJTUlURUQgVE8gVEhFIFdBUlJBTlRJRVMgT0YgTUVSQ0hBTlRBQklMSVRZLFxuLy8gRklUTkVTUyBGT1IgQSBQQVJUSUNVTEFSIFBVUlBPU0UgQU5EIE5PTklORlJJTkdFTUVOVC4gSU4gTk8gRVZFTlQgU0hBTEwgVEhFXG4vLyBBVVRIT1JTIE9SIENPUFlSSUdIVCBIT0xERVJTIEJFIExJQUJMRSBGT1IgQU5ZIENMQUlNLCBEQU1BR0VTIE9SIE9USEVSXG4vLyBMSUFCSUxJVFksIFdIRVRIRVIgSU4gQU4gQUNUSU9OIE9GIENPTlRSQUNULCBUT1JUIE9SIE9USEVSV0lTRSwgQVJJU0lORyBGUk9NLFxuLy8gT1VUIE9GIE9SIElOIENPTk5FQ1RJT04gV0lUSCBUSEUgU09GVFdBUkUgT1IgVEhFIFVTRSBPUiBPVEhFUiBERUFMSU5HUyBJTiBUSEVcbi8vIFNPRlRXQVJFLlxuXG4vLyBDb3B5cmlnaHQgKEMpIDwyMDE4PiBJbnRlbCBDb3Jwb3JhdGlvblxuLy9cbi8vIFNQRFgtTGljZW5zZS1JZGVudGlmaWVyOiBBcGFjaGUtMi4wXG5cbi8vIFRoaXMgZmlsZSBpcyBib3Jyb3dlZCBmcm9tIGx5bmNraWEvbGljb2RlIHdpdGggc29tZSBtb2RpZmljYXRpb25zLlxuXG4vKiBnbG9iYWwgd2luZG93ICovXG5cbid1c2Ugc3RyaWN0JztcblxuLypcbiAqIEFQSSB0byB3cml0ZSBsb2dzIGJhc2VkIG9uIHRyYWRpdGlvbmFsIGxvZ2dpbmcgbWVjaGFuaXNtczogZGVidWcsIHRyYWNlLFxuICogaW5mbywgd2FybmluZywgZXJyb3JcbiAqL1xuY29uc3QgTG9nZ2VyID0gKGZ1bmN0aW9uKCkge1xuICBjb25zdCBERUJVRyA9IDA7XG4gIGNvbnN0IFRSQUNFID0gMTtcbiAgY29uc3QgSU5GTyA9IDI7XG4gIGNvbnN0IFdBUk5JTkcgPSAzO1xuICBjb25zdCBFUlJPUiA9IDQ7XG4gIGNvbnN0IE5PTkUgPSA1O1xuXG4gIGNvbnN0IG5vT3AgPSBmdW5jdGlvbigpIHt9O1xuXG4gIC8vIHx0aGF0fCBpcyB0aGUgb2JqZWN0IHRvIGJlIHJldHVybmVkLlxuICBjb25zdCB0aGF0ID0ge1xuICAgIERFQlVHOiBERUJVRyxcbiAgICBUUkFDRTogVFJBQ0UsXG4gICAgSU5GTzogSU5GTyxcbiAgICBXQVJOSU5HOiBXQVJOSU5HLFxuICAgIEVSUk9SOiBFUlJPUixcbiAgICBOT05FOiBOT05FLFxuICB9O1xuXG4gIHRoYXQubG9nID0gKC4uLmFyZ3MpID0+IHtcbiAgICB3aW5kb3cuY29uc29sZS5sb2coKG5ldyBEYXRlKCkpLnRvSVNPU3RyaW5nKCksIC4uLmFyZ3MpO1xuICB9O1xuXG4gIGNvbnN0IGJpbmRUeXBlID0gZnVuY3Rpb24odHlwZSkge1xuICAgIGlmICh0eXBlb2Ygd2luZG93LmNvbnNvbGVbdHlwZV0gPT09ICdmdW5jdGlvbicpIHtcbiAgICAgIHJldHVybiAoLi4uYXJncykgPT4ge1xuICAgICAgICB3aW5kb3cuY29uc29sZVt0eXBlXSgobmV3IERhdGUoKSkudG9JU09TdHJpbmcoKSwgLi4uYXJncyk7XG4gICAgICB9O1xuICAgIH0gZWxzZSB7XG4gICAgICByZXR1cm4gdGhhdC5sb2c7XG4gICAgfVxuICB9O1xuXG4gIGNvbnN0IHNldExvZ0xldmVsID0gZnVuY3Rpb24obGV2ZWwpIHtcbiAgICBpZiAobGV2ZWwgPD0gREVCVUcpIHtcbiAgICAgIHRoYXQuZGVidWcgPSBiaW5kVHlwZSgnZGVidWcnKTtcbiAgICB9IGVsc2Uge1xuICAgICAgdGhhdC5kZWJ1ZyA9IG5vT3A7XG4gICAgfVxuICAgIGlmIChsZXZlbCA8PSBUUkFDRSkge1xuICAgICAgdGhhdC50cmFjZSA9IGJpbmRUeXBlKCd0cmFjZScpO1xuICAgIH0gZWxzZSB7XG4gICAgICB0aGF0LnRyYWNlID0gbm9PcDtcbiAgICB9XG4gICAgaWYgKGxldmVsIDw9IElORk8pIHtcbiAgICAgIHRoYXQuaW5mbyA9IGJpbmRUeXBlKCdpbmZvJyk7XG4gICAgfSBlbHNlIHtcbiAgICAgIHRoYXQuaW5mbyA9IG5vT3A7XG4gICAgfVxuICAgIGlmIChsZXZlbCA8PSBXQVJOSU5HKSB7XG4gICAgICB0aGF0Lndhcm5pbmcgPSBiaW5kVHlwZSgnd2FybicpO1xuICAgIH0gZWxzZSB7XG4gICAgICB0aGF0Lndhcm5pbmcgPSBub09wO1xuICAgIH1cbiAgICBpZiAobGV2ZWwgPD0gRVJST1IpIHtcbiAgICAgIHRoYXQuZXJyb3IgPSBiaW5kVHlwZSgnZXJyb3InKTtcbiAgICB9IGVsc2Uge1xuICAgICAgdGhhdC5lcnJvciA9IG5vT3A7XG4gICAgfVxuICB9O1xuXG4gIHNldExvZ0xldmVsKERFQlVHKTsgLy8gRGVmYXVsdCBsZXZlbCBpcyBkZWJ1Zy5cblxuICB0aGF0LnNldExvZ0xldmVsID0gc2V0TG9nTGV2ZWw7XG5cbiAgcmV0dXJuIHRoYXQ7XG59KCkpO1xuXG5leHBvcnQgZGVmYXVsdCBMb2dnZXI7XG4iLCIvLyBDb3B5cmlnaHQgKEMpIDwyMDE4PiBJbnRlbCBDb3Jwb3JhdGlvblxuLy9cbi8vIFNQRFgtTGljZW5zZS1JZGVudGlmaWVyOiBBcGFjaGUtMi4wXG5cbid1c2Ugc3RyaWN0Jztcbi8qKlxuICogQGNsYXNzIEF1ZGlvU291cmNlSW5mb1xuICogQGNsYXNzRGVzYyBTb3VyY2UgaW5mbyBhYm91dCBhbiBhdWRpbyB0cmFjay4gVmFsdWVzOiAnbWljJywgJ3NjcmVlbi1jYXN0JyxcbiAqICdmaWxlJywgJ21peGVkJy5cbiAqIEBtZW1iZXJPZiBPd3QuQmFzZVxuICogQHJlYWRvbmx5XG4gKiBAZW51bSB7c3RyaW5nfVxuICovXG5leHBvcnQgY29uc3QgQXVkaW9Tb3VyY2VJbmZvID0ge1xuICBNSUM6ICdtaWMnLFxuICBTQ1JFRU5DQVNUOiAnc2NyZWVuLWNhc3QnLFxuICBGSUxFOiAnZmlsZScsXG4gIE1JWEVEOiAnbWl4ZWQnLFxufTtcblxuLyoqXG4gKiBAY2xhc3MgVmlkZW9Tb3VyY2VJbmZvXG4gKiBAY2xhc3NEZXNjIFNvdXJjZSBpbmZvIGFib3V0IGEgdmlkZW8gdHJhY2suIFZhbHVlczogJ2NhbWVyYScsICdzY3JlZW4tY2FzdCcsXG4gKiAnZmlsZScsICdtaXhlZCcuXG4gKiBAbWVtYmVyT2YgT3d0LkJhc2VcbiAqIEByZWFkb25seVxuICogQGVudW0ge3N0cmluZ31cbiAqL1xuZXhwb3J0IGNvbnN0IFZpZGVvU291cmNlSW5mbyA9IHtcbiAgQ0FNRVJBOiAnY2FtZXJhJyxcbiAgU0NSRUVOQ0FTVDogJ3NjcmVlbi1jYXN0JyxcbiAgRklMRTogJ2ZpbGUnLFxuICBNSVhFRDogJ21peGVkJyxcbn07XG5cbi8qKlxuICogQGNsYXNzIFRyYWNrS2luZFxuICogQGNsYXNzRGVzYyBLaW5kIG9mIGEgdHJhY2suIFZhbHVlczogJ2F1ZGlvJyBmb3IgYXVkaW8gdHJhY2ssICd2aWRlbycgZm9yXG4gKiB2aWRlbyB0cmFjaywgJ2F2JyBmb3IgYm90aCBhdWRpbyBhbmQgdmlkZW8gdHJhY2tzLlxuICogQG1lbWJlck9mIE93dC5CYXNlXG4gKiBAcmVhZG9ubHlcbiAqIEBlbnVtIHtzdHJpbmd9XG4gKi9cbmV4cG9ydCBjb25zdCBUcmFja0tpbmQgPSB7XG4gIC8qKlxuICAgKiBBdWRpbyB0cmFja3MuXG4gICAqIEB0eXBlIHN0cmluZ1xuICAgKi9cbiAgQVVESU86ICdhdWRpbycsXG4gIC8qKlxuICAgKiBWaWRlbyB0cmFja3MuXG4gICAqIEB0eXBlIHN0cmluZ1xuICAgKi9cbiAgVklERU86ICd2aWRlbycsXG4gIC8qKlxuICAgKiBCb3RoIGF1ZGlvIGFuZCB2aWRlbyB0cmFja3MuXG4gICAqIEB0eXBlIHN0cmluZ1xuICAgKi9cbiAgQVVESU9fQU5EX1ZJREVPOiAnYXYnLFxufTtcbi8qKlxuICogQGNsYXNzIFJlc29sdXRpb25cbiAqIEBtZW1iZXJPZiBPd3QuQmFzZVxuICogQGNsYXNzRGVzYyBUaGUgUmVzb2x1dGlvbiBkZWZpbmVzIHRoZSBzaXplIG9mIGEgcmVjdGFuZ2xlLlxuICogQGNvbnN0cnVjdG9yXG4gKiBAcGFyYW0ge251bWJlcn0gd2lkdGhcbiAqIEBwYXJhbSB7bnVtYmVyfSBoZWlnaHRcbiAqL1xuZXhwb3J0IGNsYXNzIFJlc29sdXRpb24ge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3Rvcih3aWR0aCwgaGVpZ2h0KSB7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7bnVtYmVyfSB3aWR0aFxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5SZXNvbHV0aW9uXG4gICAgICovXG4gICAgdGhpcy53aWR0aCA9IHdpZHRoO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge251bWJlcn0gaGVpZ2h0XG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlJlc29sdXRpb25cbiAgICAgKi9cbiAgICB0aGlzLmhlaWdodCA9IGhlaWdodDtcbiAgfVxufVxuIiwiLy8gQ29weXJpZ2h0IChDKSA8MjAxOD4gSW50ZWwgQ29ycG9yYXRpb25cbi8vXG4vLyBTUERYLUxpY2Vuc2UtSWRlbnRpZmllcjogQXBhY2hlLTIuMFxuXG4vKiBnbG9iYWwgUHJvbWlzZSwgbmF2aWdhdG9yICovXG5cbid1c2Ugc3RyaWN0JztcbmltcG9ydCAqIGFzIHV0aWxzIGZyb20gJy4vdXRpbHMuanMnO1xuaW1wb3J0ICogYXMgTWVkaWFGb3JtYXRNb2R1bGUgZnJvbSAnLi9tZWRpYWZvcm1hdC5qcyc7XG5cbi8qKlxuICogQGNsYXNzIEF1ZGlvVHJhY2tDb25zdHJhaW50c1xuICogQGNsYXNzRGVzYyBDb25zdHJhaW50cyBmb3IgY3JlYXRpbmcgYW4gYXVkaW8gTWVkaWFTdHJlYW1UcmFjay5cbiAqIEBtZW1iZXJvZiBPd3QuQmFzZVxuICogQGNvbnN0cnVjdG9yXG4gKiBAcGFyYW0ge093dC5CYXNlLkF1ZGlvU291cmNlSW5mb30gc291cmNlIFNvdXJjZSBpbmZvIG9mIHRoaXMgYXVkaW8gdHJhY2suXG4gKi9cbmV4cG9ydCBjbGFzcyBBdWRpb1RyYWNrQ29uc3RyYWludHMge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3Rvcihzb3VyY2UpIHtcbiAgICBpZiAoIU9iamVjdC52YWx1ZXMoTWVkaWFGb3JtYXRNb2R1bGUuQXVkaW9Tb3VyY2VJbmZvKVxuICAgICAgICAuc29tZSgodikgPT4gdiA9PT0gc291cmNlKSkge1xuICAgICAgdGhyb3cgbmV3IFR5cGVFcnJvcignSW52YWxpZCBzb3VyY2UuJyk7XG4gICAgfVxuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge3N0cmluZ30gc291cmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLkF1ZGlvVHJhY2tDb25zdHJhaW50c1xuICAgICAqIEBkZXNjIFZhbHVlcyBjb3VsZCBiZSBcIm1pY1wiLCBcInNjcmVlbi1jYXN0XCIsIFwiZmlsZVwiIG9yIFwibWl4ZWRcIi5cbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKi9cbiAgICB0aGlzLnNvdXJjZSA9IHNvdXJjZTtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHtzdHJpbmd9IGRldmljZUlkXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLkF1ZGlvVHJhY2tDb25zdHJhaW50c1xuICAgICAqIEBkZXNjIERvIG5vdCBwcm92aWRlIGRldmljZUlkIGlmIHNvdXJjZSBpcyBub3QgXCJtaWNcIi5cbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAc2VlIGh0dHBzOi8vdzNjLmdpdGh1Yi5pby9tZWRpYWNhcHR1cmUtbWFpbi8jZGVmLWNvbnN0cmFpbnQtZGV2aWNlSWRcbiAgICAgKi9cbiAgICB0aGlzLmRldmljZUlkID0gdW5kZWZpbmVkO1xuICB9XG59XG5cbi8qKlxuICogQGNsYXNzIFZpZGVvVHJhY2tDb25zdHJhaW50c1xuICogQGNsYXNzRGVzYyBDb25zdHJhaW50cyBmb3IgY3JlYXRpbmcgYSB2aWRlbyBNZWRpYVN0cmVhbVRyYWNrLlxuICogQG1lbWJlcm9mIE93dC5CYXNlXG4gKiBAY29uc3RydWN0b3JcbiAqIEBwYXJhbSB7T3d0LkJhc2UuVmlkZW9Tb3VyY2VJbmZvfSBzb3VyY2UgU291cmNlIGluZm8gb2YgdGhpcyB2aWRlbyB0cmFjay5cbiAqL1xuZXhwb3J0IGNsYXNzIFZpZGVvVHJhY2tDb25zdHJhaW50cyB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKHNvdXJjZSkge1xuICAgIGlmICghT2JqZWN0LnZhbHVlcyhNZWRpYUZvcm1hdE1vZHVsZS5WaWRlb1NvdXJjZUluZm8pXG4gICAgICAgIC5zb21lKCh2KSA9PiB2ID09PSBzb3VyY2UpKSB7XG4gICAgICB0aHJvdyBuZXcgVHlwZUVycm9yKCdJbnZhbGlkIHNvdXJjZS4nKTtcbiAgICB9XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7c3RyaW5nfSBzb3VyY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuVmlkZW9UcmFja0NvbnN0cmFpbnRzXG4gICAgICogQGRlc2MgVmFsdWVzIGNvdWxkIGJlIFwiY2FtZXJhXCIsIFwic2NyZWVuLWNhc3RcIiwgXCJmaWxlXCIgb3IgXCJtaXhlZFwiLlxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqL1xuICAgIHRoaXMuc291cmNlID0gc291cmNlO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge3N0cmluZ30gZGV2aWNlSWRcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuVmlkZW9UcmFja0NvbnN0cmFpbnRzXG4gICAgICogQGRlc2MgRG8gbm90IHByb3ZpZGUgZGV2aWNlSWQgaWYgc291cmNlIGlzIG5vdCBcImNhbWVyYVwiLlxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBzZWUgaHR0cHM6Ly93M2MuZ2l0aHViLmlvL21lZGlhY2FwdHVyZS1tYWluLyNkZWYtY29uc3RyYWludC1kZXZpY2VJZFxuICAgICAqL1xuXG4gICAgdGhpcy5kZXZpY2VJZCA9IHVuZGVmaW5lZDtcblxuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge093dC5CYXNlLlJlc29sdXRpb259IHJlc29sdXRpb25cbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuVmlkZW9UcmFja0NvbnN0cmFpbnRzXG4gICAgICogQGluc3RhbmNlXG4gICAgICovXG4gICAgdGhpcy5yZXNvbHV0aW9uID0gdW5kZWZpbmVkO1xuXG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7bnVtYmVyfSBmcmFtZVJhdGVcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuVmlkZW9UcmFja0NvbnN0cmFpbnRzXG4gICAgICogQGluc3RhbmNlXG4gICAgICovXG4gICAgdGhpcy5mcmFtZVJhdGUgPSB1bmRlZmluZWQ7XG4gIH1cbn1cbi8qKlxuICogQGNsYXNzIFN0cmVhbUNvbnN0cmFpbnRzXG4gKiBAY2xhc3NEZXNjIENvbnN0cmFpbnRzIGZvciBjcmVhdGluZyBhIE1lZGlhU3RyZWFtIGZyb20gc2NyZWVuIG1pYyBhbmQgY2FtZXJhLlxuICogQG1lbWJlcm9mIE93dC5CYXNlXG4gKiBAY29uc3RydWN0b3JcbiAqIEBwYXJhbSB7P093dC5CYXNlLkF1ZGlvVHJhY2tDb25zdHJhaW50c30gYXVkaW9Db25zdHJhaW50c1xuICogQHBhcmFtIHs/T3d0LkJhc2UuVmlkZW9UcmFja0NvbnN0cmFpbnRzfSB2aWRlb0NvbnN0cmFpbnRzXG4gKi9cbmV4cG9ydCBjbGFzcyBTdHJlYW1Db25zdHJhaW50cyB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKGF1ZGlvQ29uc3RyYWludHMgPSBmYWxzZSwgdmlkZW9Db25zdHJhaW50cyA9IGZhbHNlKSB7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7T3d0LkJhc2UuTWVkaWFTdHJlYW1UcmFja0RldmljZUNvbnN0cmFpbnRzRm9yQXVkaW99IGF1ZGlvXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLk1lZGlhU3RyZWFtRGV2aWNlQ29uc3RyYWludHNcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKi9cbiAgICB0aGlzLmF1ZGlvID0gYXVkaW9Db25zdHJhaW50cztcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHtPd3QuQmFzZS5NZWRpYVN0cmVhbVRyYWNrRGV2aWNlQ29uc3RyYWludHNGb3JWaWRlb30gVmlkZW9cbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuTWVkaWFTdHJlYW1EZXZpY2VDb25zdHJhaW50c1xuICAgICAqIEBpbnN0YW5jZVxuICAgICAqL1xuICAgIHRoaXMudmlkZW8gPSB2aWRlb0NvbnN0cmFpbnRzO1xuICB9XG59XG5cbi8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG5mdW5jdGlvbiBpc1ZpZGVvQ29uc3RyYWluc0ZvclNjcmVlbkNhc3QoY29uc3RyYWludHMpIHtcbiAgcmV0dXJuICh0eXBlb2YgY29uc3RyYWludHMudmlkZW8gPT09ICdvYmplY3QnICYmIGNvbnN0cmFpbnRzLnZpZGVvLnNvdXJjZSA9PT1cbiAgICAgIE1lZGlhRm9ybWF0TW9kdWxlLlZpZGVvU291cmNlSW5mby5TQ1JFRU5DQVNUKTtcbn1cblxuLyoqXG4gKiBAY2xhc3MgTWVkaWFTdHJlYW1GYWN0b3J5XG4gKiBAY2xhc3NEZXNjIEEgZmFjdG9yeSB0byBjcmVhdGUgTWVkaWFTdHJlYW0uIFlvdSBjYW4gYWxzbyBjcmVhdGUgTWVkaWFTdHJlYW1cbiAqIGJ5IHlvdXJzZWxmLlxuICogQG1lbWJlcm9mIE93dC5CYXNlXG4gKi9cbmV4cG9ydCBjbGFzcyBNZWRpYVN0cmVhbUZhY3Rvcnkge1xuICAvKipcbiAgICogQGZ1bmN0aW9uIGNyZWF0ZU1lZGlhU3RyZWFtXG4gICAqIEBzdGF0aWNcbiAgICogQGRlc2MgQ3JlYXRlIGEgTWVkaWFTdHJlYW0gd2l0aCBnaXZlbiBjb25zdHJhaW50cy4gSWYgeW91IHdhbnQgdG8gY3JlYXRlIGFcbiAgICogTWVkaWFTdHJlYW0gZm9yIHNjcmVlbiBjYXN0LCBwbGVhc2UgbWFrZSBzdXJlIGJvdGggYXVkaW8gYW5kIHZpZGVvJ3Mgc291cmNlXG4gICAqIGFyZSBcInNjcmVlbi1jYXN0XCIuXG4gICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5NZWRpYVN0cmVhbUZhY3RvcnlcbiAgICogQHJldHVybiB7UHJvbWlzZTxNZWRpYVN0cmVhbSwgRXJyb3I+fSBSZXR1cm4gYSBwcm9taXNlIHRoYXQgaXMgcmVzb2x2ZWRcbiAgICogd2hlbiBzdHJlYW0gaXMgc3VjY2Vzc2Z1bGx5IGNyZWF0ZWQsIG9yIHJlamVjdGVkIGlmIG9uZSBvZiB0aGUgZm9sbG93aW5nXG4gICAqIGVycm9yIGhhcHBlbmVkOlxuICAgKiAtIE9uZSBvciBtb3JlIHBhcmFtZXRlcnMgY2Fubm90IGJlIHNhdGlzZmllZC5cbiAgICogLSBTcGVjaWZpZWQgZGV2aWNlIGlzIGJ1c3kuXG4gICAqIC0gQ2Fubm90IG9idGFpbiBuZWNlc3NhcnkgcGVybWlzc2lvbiBvciBvcGVyYXRpb24gaXMgY2FuY2VsZWQgYnkgdXNlci5cbiAgICogLSBWaWRlbyBzb3VyY2UgaXMgc2NyZWVuIGNhc3QsIHdoaWxlIGF1ZGlvIHNvdXJjZSBpcyBub3QuXG4gICAqIC0gQXVkaW8gc291cmNlIGlzIHNjcmVlbiBjYXN0LCB3aGlsZSB2aWRlbyBzb3VyY2UgaXMgZGlzYWJsZWQuXG4gICAqIEBwYXJhbSB7T3d0LkJhc2UuU3RyZWFtQ29uc3RyYWludHN9IGNvbnN0cmFpbnRzXG4gICAqL1xuICBzdGF0aWMgY3JlYXRlTWVkaWFTdHJlYW0oY29uc3RyYWludHMpIHtcbiAgICBpZiAodHlwZW9mIGNvbnN0cmFpbnRzICE9PSAnb2JqZWN0JyB8fFxuICAgICAgICAoIWNvbnN0cmFpbnRzLmF1ZGlvICYmICFjb25zdHJhaW50cy52aWRlbykpIHtcbiAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgVHlwZUVycm9yKCdJbnZhbGlkIGNvbnN0cmFpbnMnKSk7XG4gICAgfVxuICAgIGlmICghaXNWaWRlb0NvbnN0cmFpbnNGb3JTY3JlZW5DYXN0KGNvbnN0cmFpbnRzKSAmJlxuICAgICAgICAodHlwZW9mIGNvbnN0cmFpbnRzLmF1ZGlvID09PSAnb2JqZWN0JykgJiZcbiAgICAgICAgY29uc3RyYWludHMuYXVkaW8uc291cmNlID09PVxuICAgICAgICAgICAgTWVkaWFGb3JtYXRNb2R1bGUuQXVkaW9Tb3VyY2VJbmZvLlNDUkVFTkNBU1QpIHtcbiAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChcbiAgICAgICAgICBuZXcgVHlwZUVycm9yKCdDYW5ub3Qgc2hhcmUgc2NyZWVuIHdpdGhvdXQgdmlkZW8uJykpO1xuICAgIH1cbiAgICBpZiAoaXNWaWRlb0NvbnN0cmFpbnNGb3JTY3JlZW5DYXN0KGNvbnN0cmFpbnRzKSAmJlxuICAgICAgICB0eXBlb2YgY29uc3RyYWludHMuYXVkaW8gPT09ICdvYmplY3QnICYmXG4gICAgICAgIGNvbnN0cmFpbnRzLmF1ZGlvLnNvdXJjZSAhPT1cbiAgICAgICAgICAgIE1lZGlhRm9ybWF0TW9kdWxlLkF1ZGlvU291cmNlSW5mby5TQ1JFRU5DQVNUKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IFR5cGVFcnJvcihcbiAgICAgICAgICAnQ2Fubm90IGNhcHR1cmUgdmlkZW8gZnJvbSBzY3JlZW4gY2FzdCB3aGlsZSBjYXB0dXJlIGF1ZGlvIGZyb20nXG4gICAgICAgICAgKyAnIG90aGVyIHNvdXJjZS4nKSk7XG4gICAgfVxuXG4gICAgLy8gQ2hlY2sgYW5kIGNvbnZlcnQgY29uc3RyYWludHMuXG4gICAgaWYgKCFjb25zdHJhaW50cy5hdWRpbyAmJiAhY29uc3RyYWludHMudmlkZW8pIHtcbiAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgVHlwZUVycm9yKFxuICAgICAgICAgICdBdCBsZWFzdCBvbmUgb2YgYXVkaW8gYW5kIHZpZGVvIG11c3QgYmUgcmVxdWVzdGVkLicpKTtcbiAgICB9XG4gICAgY29uc3QgbWVkaWFDb25zdHJhaW50cyA9IE9iamVjdC5jcmVhdGUoe30pO1xuICAgIGlmICh0eXBlb2YgY29uc3RyYWludHMuYXVkaW8gPT09ICdvYmplY3QnICYmXG4gICAgICAgIGNvbnN0cmFpbnRzLmF1ZGlvLnNvdXJjZSA9PT0gTWVkaWFGb3JtYXRNb2R1bGUuQXVkaW9Tb3VyY2VJbmZvLk1JQykge1xuICAgICAgbWVkaWFDb25zdHJhaW50cy5hdWRpbyA9IE9iamVjdC5jcmVhdGUoe30pO1xuICAgICAgaWYgKHV0aWxzLmlzRWRnZSgpKSB7XG4gICAgICAgIG1lZGlhQ29uc3RyYWludHMuYXVkaW8uZGV2aWNlSWQgPSBjb25zdHJhaW50cy5hdWRpby5kZXZpY2VJZDtcbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIG1lZGlhQ29uc3RyYWludHMuYXVkaW8uZGV2aWNlSWQgPSB7XG4gICAgICAgICAgZXhhY3Q6IGNvbnN0cmFpbnRzLmF1ZGlvLmRldmljZUlkLFxuICAgICAgICB9O1xuICAgICAgfVxuICAgIH0gZWxzZSB7XG4gICAgICBpZiAoY29uc3RyYWludHMuYXVkaW8uc291cmNlID09PVxuICAgICAgICAgIE1lZGlhRm9ybWF0TW9kdWxlLkF1ZGlvU291cmNlSW5mby5TQ1JFRU5DQVNUKSB7XG4gICAgICAgIG1lZGlhQ29uc3RyYWludHMuYXVkaW8gPSB0cnVlO1xuICAgICAgfSBlbHNlIHtcbiAgICAgICAgbWVkaWFDb25zdHJhaW50cy5hdWRpbyA9IGNvbnN0cmFpbnRzLmF1ZGlvO1xuICAgICAgfVxuICAgIH1cbiAgICBpZiAodHlwZW9mIGNvbnN0cmFpbnRzLnZpZGVvID09PSAnb2JqZWN0Jykge1xuICAgICAgbWVkaWFDb25zdHJhaW50cy52aWRlbyA9IE9iamVjdC5jcmVhdGUoe30pO1xuICAgICAgaWYgKHR5cGVvZiBjb25zdHJhaW50cy52aWRlby5mcmFtZVJhdGUgPT09ICdudW1iZXInKSB7XG4gICAgICAgIG1lZGlhQ29uc3RyYWludHMudmlkZW8uZnJhbWVSYXRlID0gY29uc3RyYWludHMudmlkZW8uZnJhbWVSYXRlO1xuICAgICAgfVxuICAgICAgaWYgKGNvbnN0cmFpbnRzLnZpZGVvLnJlc29sdXRpb24gJiZcbiAgICAgICAgICBjb25zdHJhaW50cy52aWRlby5yZXNvbHV0aW9uLndpZHRoICYmXG4gICAgICAgICAgY29uc3RyYWludHMudmlkZW8ucmVzb2x1dGlvbi5oZWlnaHQpIHtcbiAgICAgICAgaWYgKGNvbnN0cmFpbnRzLnZpZGVvLnNvdXJjZSA9PT1cbiAgICAgICAgICAgICAgTWVkaWFGb3JtYXRNb2R1bGUuVmlkZW9Tb3VyY2VJbmZvLlNDUkVFTkNBU1QpIHtcbiAgICAgICAgICBtZWRpYUNvbnN0cmFpbnRzLnZpZGVvLndpZHRoID0gY29uc3RyYWludHMudmlkZW8ucmVzb2x1dGlvbi53aWR0aDtcbiAgICAgICAgICBtZWRpYUNvbnN0cmFpbnRzLnZpZGVvLmhlaWdodCA9IGNvbnN0cmFpbnRzLnZpZGVvLnJlc29sdXRpb24uaGVpZ2h0O1xuICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgIG1lZGlhQ29uc3RyYWludHMudmlkZW8ud2lkdGggPSBPYmplY3QuY3JlYXRlKHt9KTtcbiAgICAgICAgICBtZWRpYUNvbnN0cmFpbnRzLnZpZGVvLndpZHRoLmV4YWN0ID1cbiAgICAgICAgICAgIGNvbnN0cmFpbnRzLnZpZGVvLnJlc29sdXRpb24ud2lkdGg7XG4gICAgICAgICAgbWVkaWFDb25zdHJhaW50cy52aWRlby5oZWlnaHQgPSBPYmplY3QuY3JlYXRlKHt9KTtcbiAgICAgICAgICBtZWRpYUNvbnN0cmFpbnRzLnZpZGVvLmhlaWdodC5leGFjdCA9XG4gICAgICAgICAgICBjb25zdHJhaW50cy52aWRlby5yZXNvbHV0aW9uLmhlaWdodDtcbiAgICAgICAgfVxuICAgICAgfVxuICAgICAgaWYgKHR5cGVvZiBjb25zdHJhaW50cy52aWRlby5kZXZpY2VJZCA9PT0gJ3N0cmluZycpIHtcbiAgICAgICAgbWVkaWFDb25zdHJhaW50cy52aWRlby5kZXZpY2VJZCA9IHtleGFjdDogY29uc3RyYWludHMudmlkZW8uZGV2aWNlSWR9O1xuICAgICAgfVxuICAgICAgaWYgKHV0aWxzLmlzRmlyZWZveCgpICYmXG4gICAgICAgICAgY29uc3RyYWludHMudmlkZW8uc291cmNlID09PVxuICAgICAgICAgICAgICBNZWRpYUZvcm1hdE1vZHVsZS5WaWRlb1NvdXJjZUluZm8uU0NSRUVOQ0FTVCkge1xuICAgICAgICBtZWRpYUNvbnN0cmFpbnRzLnZpZGVvLm1lZGlhU291cmNlID0gJ3NjcmVlbic7XG4gICAgICB9XG4gICAgfSBlbHNlIHtcbiAgICAgIG1lZGlhQ29uc3RyYWludHMudmlkZW8gPSBjb25zdHJhaW50cy52aWRlbztcbiAgICB9XG5cbiAgICBpZiAoaXNWaWRlb0NvbnN0cmFpbnNGb3JTY3JlZW5DYXN0KGNvbnN0cmFpbnRzKSkge1xuICAgICAgcmV0dXJuIG5hdmlnYXRvci5tZWRpYURldmljZXMuZ2V0RGlzcGxheU1lZGlhKG1lZGlhQ29uc3RyYWludHMpO1xuICAgIH0gZWxzZSB7XG4gICAgICByZXR1cm4gbmF2aWdhdG9yLm1lZGlhRGV2aWNlcy5nZXRVc2VyTWVkaWEobWVkaWFDb25zdHJhaW50cyk7XG4gICAgfVxuICB9XG59XG4iLCIvLyBDb3B5cmlnaHQgKEMpIDwyMDE4PiBJbnRlbCBDb3Jwb3JhdGlvblxuLy9cbi8vIFNQRFgtTGljZW5zZS1JZGVudGlmaWVyOiBBcGFjaGUtMi4wXG5cbid1c2Ugc3RyaWN0JztcblxuaW1wb3J0ICogYXMgVXRpbHMgZnJvbSAnLi91dGlscy5qcyc7XG5pbXBvcnQge0V2ZW50RGlzcGF0Y2hlcn0gZnJvbSAnLi4vYmFzZS9ldmVudC5qcyc7XG5cbi8qKlxuICogQGNsYXNzIEF1ZGlvUHVibGljYXRpb25TZXR0aW5nc1xuICogQG1lbWJlck9mIE93dC5CYXNlXG4gKiBAY2xhc3NEZXNjIFRoZSBhdWRpbyBzZXR0aW5ncyBvZiBhIHB1YmxpY2F0aW9uLlxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY2xhc3MgQXVkaW9QdWJsaWNhdGlvblNldHRpbmdzIHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IoY29kZWMpIHtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/T3d0LkJhc2UuQXVkaW9Db2RlY1BhcmFtZXRlcnN9IGNvZGVjXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLkF1ZGlvUHVibGljYXRpb25TZXR0aW5nc1xuICAgICAqL1xuICAgIHRoaXMuY29kZWMgPSBjb2RlYztcbiAgfVxufVxuXG4vKipcbiAqIEBjbGFzcyBWaWRlb1B1YmxpY2F0aW9uU2V0dGluZ3NcbiAqIEBtZW1iZXJPZiBPd3QuQmFzZVxuICogQGNsYXNzRGVzYyBUaGUgdmlkZW8gc2V0dGluZ3Mgb2YgYSBwdWJsaWNhdGlvbi5cbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNsYXNzIFZpZGVvUHVibGljYXRpb25TZXR0aW5ncyB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKGNvZGVjLCByZXNvbHV0aW9uLCBmcmFtZVJhdGUsXG4gICAgICBiaXRyYXRlLCBrZXlGcmFtZUludGVydmFsLCByaWQpIHtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/T3d0LkJhc2UuVmlkZW9Db2RlY1BhcmFtZXRlcnN9IGNvZGVjXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlZpZGVvUHVibGljYXRpb25TZXR0aW5nc1xuICAgICAqL1xuICAgIHRoaXMuY29kZWM9Y29kZWMsXG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P093dC5CYXNlLlJlc29sdXRpb259IHJlc29sdXRpb25cbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuVmlkZW9QdWJsaWNhdGlvblNldHRpbmdzXG4gICAgICovXG4gICAgdGhpcy5yZXNvbHV0aW9uPXJlc29sdXRpb247XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P251bWJlcn0gZnJhbWVSYXRlc1xuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBjbGFzc0Rlc2MgRnJhbWVzIHBlciBzZWNvbmQuXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlZpZGVvUHVibGljYXRpb25TZXR0aW5nc1xuICAgICAqL1xuICAgIHRoaXMuZnJhbWVSYXRlPWZyYW1lUmF0ZTtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/bnVtYmVyfSBiaXRyYXRlXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlZpZGVvUHVibGljYXRpb25TZXR0aW5nc1xuICAgICAqL1xuICAgIHRoaXMuYml0cmF0ZT1iaXRyYXRlO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIgez9udW1iZXJ9IGtleUZyYW1lSW50ZXJ2YWxzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQGNsYXNzRGVzYyBUaGUgdGltZSBpbnRlcnZhbCBiZXR3ZWVuIGtleSBmcmFtZXMuIFVuaXQ6IHNlY29uZC5cbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuVmlkZW9QdWJsaWNhdGlvblNldHRpbmdzXG4gICAgICovXG4gICAgdGhpcy5rZXlGcmFtZUludGVydmFsPWtleUZyYW1lSW50ZXJ2YWw7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P251bWJlcn0gcmlkXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQGNsYXNzRGVzYyBSZXN0cmljdGlvbiBpZGVudGlmaWVyIHRvIGlkZW50aWZ5IHRoZSBSVFAgU3RyZWFtcyB3aXRoaW4gYW4gUlRQIHNlc3Npb24uXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlZpZGVvUHVibGljYXRpb25TZXR0aW5nc1xuICAgICAqL1xuICAgIHRoaXMucmlkPXJpZDtcbiAgfVxufVxuXG4vKipcbiAqIEBjbGFzcyBQdWJsaWNhdGlvblNldHRpbmdzXG4gKiBAbWVtYmVyT2YgT3d0LkJhc2VcbiAqIEBjbGFzc0Rlc2MgVGhlIHNldHRpbmdzIG9mIGEgcHVibGljYXRpb24uXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKi9cbmV4cG9ydCBjbGFzcyBQdWJsaWNhdGlvblNldHRpbmdzIHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IoYXVkaW8sIHZpZGVvKSB7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7T3d0LkJhc2UuQXVkaW9QdWJsaWNhdGlvblNldHRpbmdzW119IGF1ZGlvXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlB1YmxpY2F0aW9uU2V0dGluZ3NcbiAgICAgKi9cbiAgICB0aGlzLmF1ZGlvPWF1ZGlvO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge093dC5CYXNlLlZpZGVvUHVibGljYXRpb25TZXR0aW5nc1tdfSB2aWRlb1xuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5QdWJsaWNhdGlvblNldHRpbmdzXG4gICAgICovXG4gICAgdGhpcy52aWRlbz12aWRlbztcbiAgfVxufVxuXG4vKipcbiAqIEBjbGFzcyBQdWJsaWNhdGlvblxuICogQGV4dGVuZHMgT3d0LkJhc2UuRXZlbnREaXNwYXRjaGVyXG4gKiBAbWVtYmVyT2YgT3d0LkJhc2VcbiAqIEBjbGFzc0Rlc2MgUHVibGljYXRpb24gcmVwcmVzZW50cyBhIHNlbmRlciBmb3IgcHVibGlzaGluZyBhIHN0cmVhbS4gSXRcbiAqIGhhbmRsZXMgdGhlIGFjdGlvbnMgb24gYSBMb2NhbFN0cmVhbSBwdWJsaXNoZWQgdG8gYSBjb25mZXJlbmNlLlxuICpcbiAqIEV2ZW50czpcbiAqXG4gKiB8IEV2ZW50IE5hbWUgICAgICB8IEFyZ3VtZW50IFR5cGUgICAgfCBGaXJlZCB3aGVuICAgICAgIHxcbiAqIHwgLS0tLS0tLS0tLS0tLS0tLXwgLS0tLS0tLS0tLS0tLS0tLSB8IC0tLS0tLS0tLS0tLS0tLS0gfFxuICogfCBlbmRlZCAgICAgICAgICAgfCBFdmVudCAgICAgICAgICAgIHwgUHVibGljYXRpb24gaXMgZW5kZWQuIHxcbiAqIHwgZXJyb3IgICAgICAgICAgIHwgRXJyb3JFdmVudCAgICAgICB8IEFuIGVycm9yIG9jY3VycmVkIG9uIHRoZSBwdWJsaWNhdGlvbi4gfFxuICogfCBtdXRlICAgICAgICAgICAgfCBNdXRlRXZlbnQgICAgICAgIHwgUHVibGljYXRpb24gaXMgbXV0ZWQuIENsaWVudCBzdG9wcGVkIHNlbmRpbmcgYXVkaW8gYW5kL29yIHZpZGVvIGRhdGEgdG8gcmVtb3RlIGVuZHBvaW50LiB8XG4gKiB8IHVubXV0ZSAgICAgICAgICB8IE11dGVFdmVudCAgICAgICAgfCBQdWJsaWNhdGlvbiBpcyB1bm11dGVkLiBDbGllbnQgY29udGludWVkIHNlbmRpbmcgYXVkaW8gYW5kL29yIHZpZGVvIGRhdGEgdG8gcmVtb3RlIGVuZHBvaW50LiB8XG4gKlxuICogYGVuZGVkYCBldmVudCBtYXkgbm90IGJlIGZpcmVkIG9uIFNhZmFyaSBhZnRlciBjYWxsaW5nIGBQdWJsaWNhdGlvbi5zdG9wKClgLlxuICpcbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNsYXNzIFB1YmxpY2F0aW9uIGV4dGVuZHMgRXZlbnREaXNwYXRjaGVyIHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IoaWQsIHRyYW5zcG9ydCwgc3RvcCwgZ2V0U3RhdHMsIG11dGUsIHVubXV0ZSkge1xuICAgIHN1cGVyKCk7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7c3RyaW5nfSBpZFxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5QdWJsaWNhdGlvblxuICAgICAqL1xuICAgIE9iamVjdC5kZWZpbmVQcm9wZXJ0eSh0aGlzLCAnaWQnLCB7XG4gICAgICBjb25maWd1cmFibGU6IGZhbHNlLFxuICAgICAgd3JpdGFibGU6IGZhbHNlLFxuICAgICAgdmFsdWU6IGlkID8gaWQgOiBVdGlscy5jcmVhdGVVdWlkKCksXG4gICAgfSk7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7T3d0LkJhc2UuVHJhbnNwb3J0U2V0dGluZ3N9IHRyYW5zcG9ydFxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEByZWFkb25seVxuICAgICAqIEBkZXNjIFRyYW5zcG9ydCBzZXR0aW5ncyBmb3IgdGhlIHB1YmxpY2F0aW9uLlxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5QdWJsaWNhdGlvblxuICAgICAqL1xuICAgIE9iamVjdC5kZWZpbmVQcm9wZXJ0eSh0aGlzLCAndHJhbnNwb3J0Jywge1xuICAgICAgY29uZmlndXJhYmxlOiBmYWxzZSxcbiAgICAgIHdyaXRhYmxlOiBmYWxzZSxcbiAgICAgIHZhbHVlOiB0cmFuc3BvcnQsXG4gICAgfSk7XG4gICAgLyoqXG4gICAgICogQGZ1bmN0aW9uIHN0b3BcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAZGVzYyBTdG9wIGNlcnRhaW4gcHVibGljYXRpb24uIE9uY2UgYSBzdWJzY3JpcHRpb24gaXMgc3RvcHBlZCwgaXQgY2Fubm90XG4gICAgICogYmUgcmVjb3ZlcmVkLlxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5QdWJsaWNhdGlvblxuICAgICAqIEByZXR1cm5zIHt1bmRlZmluZWR9XG4gICAgICovXG4gICAgdGhpcy5zdG9wID0gc3RvcDtcbiAgICAvKipcbiAgICAgKiBAZnVuY3Rpb24gZ2V0U3RhdHNcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAZGVzYyBHZXQgc3RhdHMgb2YgdW5kZXJseWluZyBQZWVyQ29ubmVjdGlvbi5cbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuUHVibGljYXRpb25cbiAgICAgKiBAcmV0dXJucyB7UHJvbWlzZTxSVENTdGF0c1JlcG9ydCwgRXJyb3I+fVxuICAgICAqL1xuICAgIHRoaXMuZ2V0U3RhdHMgPSBnZXRTdGF0cztcbiAgICAvKipcbiAgICAgKiBAZnVuY3Rpb24gbXV0ZVxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBkZXNjIFN0b3Agc2VuZGluZyBkYXRhIHRvIHJlbW90ZSBlbmRwb2ludC5cbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuUHVibGljYXRpb25cbiAgICAgKiBAcGFyYW0ge093dC5CYXNlLlRyYWNrS2luZCB9IGtpbmQgS2luZCBvZiB0cmFja3MgdG8gYmUgbXV0ZWQuXG4gICAgICogQHJldHVybnMge1Byb21pc2U8dW5kZWZpbmVkLCBFcnJvcj59XG4gICAgICovXG4gICAgdGhpcy5tdXRlID0gbXV0ZTtcbiAgICAvKipcbiAgICAgKiBAZnVuY3Rpb24gdW5tdXRlXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQGRlc2MgQ29udGludWUgc2VuZGluZyBkYXRhIHRvIHJlbW90ZSBlbmRwb2ludC5cbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuUHVibGljYXRpb25cbiAgICAgKiBAcGFyYW0ge093dC5CYXNlLlRyYWNrS2luZCB9IGtpbmQgS2luZCBvZiB0cmFja3MgdG8gYmUgdW5tdXRlZC5cbiAgICAgKiBAcmV0dXJucyB7UHJvbWlzZTx1bmRlZmluZWQsIEVycm9yPn1cbiAgICAgKi9cbiAgICB0aGlzLnVubXV0ZSA9IHVubXV0ZTtcbiAgfVxufVxuXG4vKipcbiAqIEBjbGFzcyBQdWJsaXNoT3B0aW9uc1xuICogQG1lbWJlck9mIE93dC5CYXNlXG4gKiBAY2xhc3NEZXNjIFB1Ymxpc2hPcHRpb25zIGRlZmluZXMgb3B0aW9ucyBmb3IgcHVibGlzaGluZyBhXG4gKiBPd3QuQmFzZS5Mb2NhbFN0cmVhbS5cbiAqL1xuZXhwb3J0IGNsYXNzIFB1Ymxpc2hPcHRpb25zIHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IoYXVkaW8sIHZpZGVvLCB0cmFuc3BvcnQpIHtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/QXJyYXk8T3d0LkJhc2UuQXVkaW9FbmNvZGluZ1BhcmFtZXRlcnM+IHwgP0FycmF5PFJUQ1J0cEVuY29kaW5nUGFyYW1ldGVycz59IGF1ZGlvXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlB1Ymxpc2hPcHRpb25zXG4gICAgICogQGRlc2MgUGFyYW1ldGVycyBmb3IgYXVkaW8gUnRwU2VuZGVyLiBQdWJsaXNoaW5nIHdpdGggUlRDUnRwRW5jb2RpbmdQYXJhbWV0ZXJzIGlzIGFuIGV4cGVyaW1lbnRhbCBmZWF0dXJlLiBJdCBpcyBzdWJqZWN0IHRvIGNoYW5nZS5cbiAgICAgKi9cbiAgICB0aGlzLmF1ZGlvID0gYXVkaW87XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P0FycmF5PE93dC5CYXNlLlZpZGVvRW5jb2RpbmdQYXJhbWV0ZXJzPiB8ID9BcnJheTxSVENSdHBFbmNvZGluZ1BhcmFtZXRlcnM+fSB2aWRlb1xuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5QdWJsaXNoT3B0aW9uc1xuICAgICAqIEBkZXNjIFBhcmFtZXRlcnMgZm9yIHZpZGVvIFJ0cFNlbmRlci4gUHVibGlzaGluZyB3aXRoIFJUQ1J0cEVuY29kaW5nUGFyYW1ldGVycyBpcyBhbiBleHBlcmltZW50YWwgZmVhdHVyZS4gSXQgaXMgc3ViamVjdCB0byBjaGFuZ2UuXG4gICAgICovXG4gICAgdGhpcy52aWRlbyA9IHZpZGVvO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIgez9Pd3QuQmFzZS5UcmFuc3BvcnRDb25zdHJhaW50c30gdHJhbnNwb3J0XG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlB1Ymxpc2hPcHRpb25zXG4gICAgICovXG4gICAgdGhpcy50cmFuc3BvcnQgPSB0cmFuc3BvcnQ7XG4gIH1cbn1cbiIsIi8qXG4gKiAgQ29weXJpZ2h0IChjKSAyMDE0IFRoZSBXZWJSVEMgcHJvamVjdCBhdXRob3JzLiBBbGwgUmlnaHRzIFJlc2VydmVkLlxuICpcbiAqICBVc2Ugb2YgdGhpcyBzb3VyY2UgY29kZSBpcyBnb3Zlcm5lZCBieSBhIEJTRC1zdHlsZSBsaWNlbnNlXG4gKiAgdGhhdCBjYW4gYmUgZm91bmQgaW4gdGhlIExJQ0VOU0UgZmlsZSBpbiB0aGUgcm9vdCBvZiB0aGUgc291cmNlXG4gKiAgdHJlZS5cbiAqL1xuXG4vKiBNb3JlIGluZm9ybWF0aW9uIGFib3V0IHRoZXNlIG9wdGlvbnMgYXQganNoaW50LmNvbS9kb2NzL29wdGlvbnMgKi9cblxuLyogZXNsaW50LWRpc2FibGUgKi9cblxuLyogZ2xvYmFscyAgYWRhcHRlciwgdHJhY2UgKi9cbi8qIGV4cG9ydGVkIHNldENvZGVjUGFyYW0sIGljZUNhbmRpZGF0ZVR5cGUsIGZvcm1hdFR5cGVQcmVmZXJlbmNlLFxuICAgbWF5YmVTZXRPcHVzT3B0aW9ucywgbWF5YmVQcmVmZXJBdWRpb1JlY2VpdmVDb2RlYyxcbiAgIG1heWJlUHJlZmVyQXVkaW9TZW5kQ29kZWMsIG1heWJlU2V0QXVkaW9SZWNlaXZlQml0UmF0ZSxcbiAgIG1heWJlU2V0QXVkaW9TZW5kQml0UmF0ZSwgbWF5YmVQcmVmZXJWaWRlb1JlY2VpdmVDb2RlYyxcbiAgIG1heWJlUHJlZmVyVmlkZW9TZW5kQ29kZWMsIG1heWJlU2V0VmlkZW9SZWNlaXZlQml0UmF0ZSxcbiAgIG1heWJlU2V0VmlkZW9TZW5kQml0UmF0ZSwgbWF5YmVTZXRWaWRlb1NlbmRJbml0aWFsQml0UmF0ZSxcbiAgIG1heWJlUmVtb3ZlVmlkZW9GZWMsIG1lcmdlQ29uc3RyYWludHMsIHJlbW92ZUNvZGVjUGFyYW0qL1xuXG4vKiBUaGlzIGZpbGUgaXMgYm9ycm93ZWQgZnJvbSBhcHBydGMgd2l0aCBzb21lIG1vZGlmaWNhdGlvbnMuICovXG4vKiBDb21taXQgaGFzaDogYzZhZjBjMjVlOWFmNTI3ZjcxYjNhY2RkNmJmYTEzODlkNzc4ZjdiZCArIFBSIDUzMCAqL1xuXG5pbXBvcnQgTG9nZ2VyIGZyb20gJy4vbG9nZ2VyLmpzJztcblxuJ3VzZSBzdHJpY3QnO1xuXG5mdW5jdGlvbiBtZXJnZUNvbnN0cmFpbnRzKGNvbnMxLCBjb25zMikge1xuICBpZiAoIWNvbnMxIHx8ICFjb25zMikge1xuICAgIHJldHVybiBjb25zMSB8fCBjb25zMjtcbiAgfVxuICBjb25zdCBtZXJnZWQgPSBjb25zMTtcbiAgZm9yIChjb25zdCBrZXkgaW4gY29uczIpIHtcbiAgICBtZXJnZWRba2V5XSA9IGNvbnMyW2tleV07XG4gIH1cbiAgcmV0dXJuIG1lcmdlZDtcbn1cblxuZnVuY3Rpb24gaWNlQ2FuZGlkYXRlVHlwZShjYW5kaWRhdGVTdHIpIHtcbiAgcmV0dXJuIGNhbmRpZGF0ZVN0ci5zcGxpdCgnICcpWzddO1xufVxuXG4vLyBUdXJucyB0aGUgbG9jYWwgdHlwZSBwcmVmZXJlbmNlIGludG8gYSBodW1hbi1yZWFkYWJsZSBzdHJpbmcuXG4vLyBOb3RlIHRoYXQgdGhpcyBtYXBwaW5nIGlzIGJyb3dzZXItc3BlY2lmaWMuXG5mdW5jdGlvbiBmb3JtYXRUeXBlUHJlZmVyZW5jZShwcmVmKSB7XG4gIGlmIChhZGFwdGVyLmJyb3dzZXJEZXRhaWxzLmJyb3dzZXIgPT09ICdjaHJvbWUnKSB7XG4gICAgc3dpdGNoIChwcmVmKSB7XG4gICAgICBjYXNlIDA6XG4gICAgICAgIHJldHVybiAnVFVSTi9UTFMnO1xuICAgICAgY2FzZSAxOlxuICAgICAgICByZXR1cm4gJ1RVUk4vVENQJztcbiAgICAgIGNhc2UgMjpcbiAgICAgICAgcmV0dXJuICdUVVJOL1VEUCc7XG4gICAgICBkZWZhdWx0OlxuICAgICAgICBicmVhaztcbiAgICB9XG4gIH0gZWxzZSBpZiAoYWRhcHRlci5icm93c2VyRGV0YWlscy5icm93c2VyID09PSAnZmlyZWZveCcpIHtcbiAgICBzd2l0Y2ggKHByZWYpIHtcbiAgICAgIGNhc2UgMDpcbiAgICAgICAgcmV0dXJuICdUVVJOL1RDUCc7XG4gICAgICBjYXNlIDU6XG4gICAgICAgIHJldHVybiAnVFVSTi9VRFAnO1xuICAgICAgZGVmYXVsdDpcbiAgICAgICAgYnJlYWs7XG4gICAgfVxuICB9XG4gIHJldHVybiAnJztcbn1cblxuZnVuY3Rpb24gbWF5YmVTZXRPcHVzT3B0aW9ucyhzZHAsIHBhcmFtcykge1xuICAvLyBTZXQgT3B1cyBpbiBTdGVyZW8sIGlmIHN0ZXJlbyBpcyB0cnVlLCB1bnNldCBpdCwgaWYgc3RlcmVvIGlzIGZhbHNlLCBhbmRcbiAgLy8gZG8gbm90aGluZyBpZiBvdGhlcndpc2UuXG4gIGlmIChwYXJhbXMub3B1c1N0ZXJlbyA9PT0gJ3RydWUnKSB7XG4gICAgc2RwID0gc2V0Q29kZWNQYXJhbShzZHAsICdvcHVzLzQ4MDAwJywgJ3N0ZXJlbycsICcxJyk7XG4gIH0gZWxzZSBpZiAocGFyYW1zLm9wdXNTdGVyZW8gPT09ICdmYWxzZScpIHtcbiAgICBzZHAgPSByZW1vdmVDb2RlY1BhcmFtKHNkcCwgJ29wdXMvNDgwMDAnLCAnc3RlcmVvJyk7XG4gIH1cblxuICAvLyBTZXQgT3B1cyBGRUMsIGlmIG9wdXNmZWMgaXMgdHJ1ZSwgdW5zZXQgaXQsIGlmIG9wdXNmZWMgaXMgZmFsc2UsIGFuZFxuICAvLyBkbyBub3RoaW5nIGlmIG90aGVyd2lzZS5cbiAgaWYgKHBhcmFtcy5vcHVzRmVjID09PSAndHJ1ZScpIHtcbiAgICBzZHAgPSBzZXRDb2RlY1BhcmFtKHNkcCwgJ29wdXMvNDgwMDAnLCAndXNlaW5iYW5kZmVjJywgJzEnKTtcbiAgfSBlbHNlIGlmIChwYXJhbXMub3B1c0ZlYyA9PT0gJ2ZhbHNlJykge1xuICAgIHNkcCA9IHJlbW92ZUNvZGVjUGFyYW0oc2RwLCAnb3B1cy80ODAwMCcsICd1c2VpbmJhbmRmZWMnKTtcbiAgfVxuXG4gIC8vIFNldCBPcHVzIERUWCwgaWYgb3B1c2R0eCBpcyB0cnVlLCB1bnNldCBpdCwgaWYgb3B1c2R0eCBpcyBmYWxzZSwgYW5kXG4gIC8vIGRvIG5vdGhpbmcgaWYgb3RoZXJ3aXNlLlxuICBpZiAocGFyYW1zLm9wdXNEdHggPT09ICd0cnVlJykge1xuICAgIHNkcCA9IHNldENvZGVjUGFyYW0oc2RwLCAnb3B1cy80ODAwMCcsICd1c2VkdHgnLCAnMScpO1xuICB9IGVsc2UgaWYgKHBhcmFtcy5vcHVzRHR4ID09PSAnZmFsc2UnKSB7XG4gICAgc2RwID0gcmVtb3ZlQ29kZWNQYXJhbShzZHAsICdvcHVzLzQ4MDAwJywgJ3VzZWR0eCcpO1xuICB9XG5cbiAgLy8gU2V0IE9wdXMgbWF4cGxheWJhY2tyYXRlLCBpZiByZXF1ZXN0ZWQuXG4gIGlmIChwYXJhbXMub3B1c01heFBicikge1xuICAgIHNkcCA9IHNldENvZGVjUGFyYW0oXG4gICAgICAgIHNkcCwgJ29wdXMvNDgwMDAnLCAnbWF4cGxheWJhY2tyYXRlJywgcGFyYW1zLm9wdXNNYXhQYnIpO1xuICB9XG4gIHJldHVybiBzZHA7XG59XG5cbmZ1bmN0aW9uIG1heWJlU2V0QXVkaW9TZW5kQml0UmF0ZShzZHAsIHBhcmFtcykge1xuICBpZiAoIXBhcmFtcy5hdWRpb1NlbmRCaXRyYXRlKSB7XG4gICAgcmV0dXJuIHNkcDtcbiAgfVxuICBMb2dnZXIuZGVidWcoJ1ByZWZlciBhdWRpbyBzZW5kIGJpdHJhdGU6ICcgKyBwYXJhbXMuYXVkaW9TZW5kQml0cmF0ZSk7XG4gIHJldHVybiBwcmVmZXJCaXRSYXRlKHNkcCwgcGFyYW1zLmF1ZGlvU2VuZEJpdHJhdGUsICdhdWRpbycpO1xufVxuXG5mdW5jdGlvbiBtYXliZVNldEF1ZGlvUmVjZWl2ZUJpdFJhdGUoc2RwLCBwYXJhbXMpIHtcbiAgaWYgKCFwYXJhbXMuYXVkaW9SZWN2Qml0cmF0ZSkge1xuICAgIHJldHVybiBzZHA7XG4gIH1cbiAgTG9nZ2VyLmRlYnVnKCdQcmVmZXIgYXVkaW8gcmVjZWl2ZSBiaXRyYXRlOiAnICsgcGFyYW1zLmF1ZGlvUmVjdkJpdHJhdGUpO1xuICByZXR1cm4gcHJlZmVyQml0UmF0ZShzZHAsIHBhcmFtcy5hdWRpb1JlY3ZCaXRyYXRlLCAnYXVkaW8nKTtcbn1cblxuZnVuY3Rpb24gbWF5YmVTZXRWaWRlb1NlbmRCaXRSYXRlKHNkcCwgcGFyYW1zKSB7XG4gIGlmICghcGFyYW1zLnZpZGVvU2VuZEJpdHJhdGUpIHtcbiAgICByZXR1cm4gc2RwO1xuICB9XG4gIExvZ2dlci5kZWJ1ZygnUHJlZmVyIHZpZGVvIHNlbmQgYml0cmF0ZTogJyArIHBhcmFtcy52aWRlb1NlbmRCaXRyYXRlKTtcbiAgcmV0dXJuIHByZWZlckJpdFJhdGUoc2RwLCBwYXJhbXMudmlkZW9TZW5kQml0cmF0ZSwgJ3ZpZGVvJyk7XG59XG5cbmZ1bmN0aW9uIG1heWJlU2V0VmlkZW9SZWNlaXZlQml0UmF0ZShzZHAsIHBhcmFtcykge1xuICBpZiAoIXBhcmFtcy52aWRlb1JlY3ZCaXRyYXRlKSB7XG4gICAgcmV0dXJuIHNkcDtcbiAgfVxuICBMb2dnZXIuZGVidWcoJ1ByZWZlciB2aWRlbyByZWNlaXZlIGJpdHJhdGU6ICcgKyBwYXJhbXMudmlkZW9SZWN2Qml0cmF0ZSk7XG4gIHJldHVybiBwcmVmZXJCaXRSYXRlKHNkcCwgcGFyYW1zLnZpZGVvUmVjdkJpdHJhdGUsICd2aWRlbycpO1xufVxuXG4vLyBBZGQgYSBiPUFTOmJpdHJhdGUgbGluZSB0byB0aGUgbT1tZWRpYVR5cGUgc2VjdGlvbi5cbmZ1bmN0aW9uIHByZWZlckJpdFJhdGUoc2RwLCBiaXRyYXRlLCBtZWRpYVR5cGUpIHtcbiAgY29uc3Qgc2RwTGluZXMgPSBzZHAuc3BsaXQoJ1xcclxcbicpO1xuXG4gIC8vIEZpbmQgbSBsaW5lIGZvciB0aGUgZ2l2ZW4gbWVkaWFUeXBlLlxuICBjb25zdCBtTGluZUluZGV4ID0gZmluZExpbmUoc2RwTGluZXMsICdtPScsIG1lZGlhVHlwZSk7XG4gIGlmIChtTGluZUluZGV4ID09PSBudWxsKSB7XG4gICAgTG9nZ2VyLmRlYnVnKCdGYWlsZWQgdG8gYWRkIGJhbmR3aWR0aCBsaW5lIHRvIHNkcCwgYXMgbm8gbS1saW5lIGZvdW5kJyk7XG4gICAgcmV0dXJuIHNkcDtcbiAgfVxuXG4gIC8vIEZpbmQgbmV4dCBtLWxpbmUgaWYgYW55LlxuICBsZXQgbmV4dE1MaW5lSW5kZXggPSBmaW5kTGluZUluUmFuZ2Uoc2RwTGluZXMsIG1MaW5lSW5kZXggKyAxLCAtMSwgJ209Jyk7XG4gIGlmIChuZXh0TUxpbmVJbmRleCA9PT0gbnVsbCkge1xuICAgIG5leHRNTGluZUluZGV4ID0gc2RwTGluZXMubGVuZ3RoO1xuICB9XG5cbiAgLy8gRmluZCBjLWxpbmUgY29ycmVzcG9uZGluZyB0byB0aGUgbS1saW5lLlxuICBjb25zdCBjTGluZUluZGV4ID0gZmluZExpbmVJblJhbmdlKHNkcExpbmVzLCBtTGluZUluZGV4ICsgMSxcbiAgICAgIG5leHRNTGluZUluZGV4LCAnYz0nKTtcbiAgaWYgKGNMaW5lSW5kZXggPT09IG51bGwpIHtcbiAgICBMb2dnZXIuZGVidWcoJ0ZhaWxlZCB0byBhZGQgYmFuZHdpZHRoIGxpbmUgdG8gc2RwLCBhcyBubyBjLWxpbmUgZm91bmQnKTtcbiAgICByZXR1cm4gc2RwO1xuICB9XG5cbiAgLy8gQ2hlY2sgaWYgYmFuZHdpZHRoIGxpbmUgYWxyZWFkeSBleGlzdHMgYmV0d2VlbiBjLWxpbmUgYW5kIG5leHQgbS1saW5lLlxuICBjb25zdCBiTGluZUluZGV4ID0gZmluZExpbmVJblJhbmdlKHNkcExpbmVzLCBjTGluZUluZGV4ICsgMSxcbiAgICAgIG5leHRNTGluZUluZGV4LCAnYj1BUycpO1xuICBpZiAoYkxpbmVJbmRleCkge1xuICAgIHNkcExpbmVzLnNwbGljZShiTGluZUluZGV4LCAxKTtcbiAgfVxuXG4gIC8vIENyZWF0ZSB0aGUgYiAoYmFuZHdpZHRoKSBzZHAgbGluZS5cbiAgY29uc3QgYndMaW5lID0gJ2I9QVM6JyArIGJpdHJhdGU7XG4gIC8vIEFzIHBlciBSRkMgNDU2NiwgdGhlIGIgbGluZSBzaG91bGQgZm9sbG93IGFmdGVyIGMtbGluZS5cbiAgc2RwTGluZXMuc3BsaWNlKGNMaW5lSW5kZXggKyAxLCAwLCBid0xpbmUpO1xuICBzZHAgPSBzZHBMaW5lcy5qb2luKCdcXHJcXG4nKTtcbiAgcmV0dXJuIHNkcDtcbn1cblxuLy8gQWRkIGFuIGE9Zm10cDogeC1nb29nbGUtbWluLWJpdHJhdGU9a2JwcyBsaW5lLCBpZiB2aWRlb1NlbmRJbml0aWFsQml0cmF0ZVxuLy8gaXMgc3BlY2lmaWVkLiBXZSdsbCBhbHNvIGFkZCBhIHgtZ29vZ2xlLW1pbi1iaXRyYXRlIHZhbHVlLCBzaW5jZSB0aGUgbWF4XG4vLyBtdXN0IGJlID49IHRoZSBtaW4uXG5mdW5jdGlvbiBtYXliZVNldFZpZGVvU2VuZEluaXRpYWxCaXRSYXRlKHNkcCwgcGFyYW1zKSB7XG4gIGxldCBpbml0aWFsQml0cmF0ZSA9IHBhcnNlSW50KHBhcmFtcy52aWRlb1NlbmRJbml0aWFsQml0cmF0ZSk7XG4gIGlmICghaW5pdGlhbEJpdHJhdGUpIHtcbiAgICByZXR1cm4gc2RwO1xuICB9XG5cbiAgLy8gVmFsaWRhdGUgdGhlIGluaXRpYWwgYml0cmF0ZSB2YWx1ZS5cbiAgbGV0IG1heEJpdHJhdGUgPSBwYXJzZUludChpbml0aWFsQml0cmF0ZSk7XG4gIGNvbnN0IGJpdHJhdGUgPSBwYXJzZUludChwYXJhbXMudmlkZW9TZW5kQml0cmF0ZSk7XG4gIGlmIChiaXRyYXRlKSB7XG4gICAgaWYgKGluaXRpYWxCaXRyYXRlID4gYml0cmF0ZSkge1xuICAgICAgTG9nZ2VyLmRlYnVnKCdDbGFtcGluZyBpbml0aWFsIGJpdHJhdGUgdG8gbWF4IGJpdHJhdGUgb2YgJyArIGJpdHJhdGUgKyAnIGticHMuJyk7XG4gICAgICBpbml0aWFsQml0cmF0ZSA9IGJpdHJhdGU7XG4gICAgICBwYXJhbXMudmlkZW9TZW5kSW5pdGlhbEJpdHJhdGUgPSBpbml0aWFsQml0cmF0ZTtcbiAgICB9XG4gICAgbWF4Qml0cmF0ZSA9IGJpdHJhdGU7XG4gIH1cblxuICBjb25zdCBzZHBMaW5lcyA9IHNkcC5zcGxpdCgnXFxyXFxuJyk7XG5cbiAgLy8gU2VhcmNoIGZvciBtIGxpbmUuXG4gIGNvbnN0IG1MaW5lSW5kZXggPSBmaW5kTGluZShzZHBMaW5lcywgJ209JywgJ3ZpZGVvJyk7XG4gIGlmIChtTGluZUluZGV4ID09PSBudWxsKSB7XG4gICAgTG9nZ2VyLmRlYnVnKCdGYWlsZWQgdG8gZmluZCB2aWRlbyBtLWxpbmUnKTtcbiAgICByZXR1cm4gc2RwO1xuICB9XG4gIC8vIEZpZ3VyZSBvdXQgdGhlIGZpcnN0IGNvZGVjIHBheWxvYWQgdHlwZSBvbiB0aGUgbT12aWRlbyBTRFAgbGluZS5cbiAgY29uc3QgdmlkZW9NTGluZSA9IHNkcExpbmVzW21MaW5lSW5kZXhdO1xuICBjb25zdCBwYXR0ZXJuID0gbmV3IFJlZ0V4cCgnbT12aWRlb1xcXFxzXFxcXGQrXFxcXHNbQS1aL10rXFxcXHMnKTtcbiAgY29uc3Qgc2VuZFBheWxvYWRUeXBlID0gdmlkZW9NTGluZS5zcGxpdChwYXR0ZXJuKVsxXS5zcGxpdCgnICcpWzBdO1xuICBjb25zdCBmbXRwTGluZSA9IHNkcExpbmVzW2ZpbmRMaW5lKHNkcExpbmVzLCAnYT1ydHBtYXAnLCBzZW5kUGF5bG9hZFR5cGUpXTtcbiAgY29uc3QgY29kZWNOYW1lID0gZm10cExpbmUuc3BsaXQoJ2E9cnRwbWFwOicgK1xuICAgICAgc2VuZFBheWxvYWRUeXBlKVsxXS5zcGxpdCgnLycpWzBdO1xuXG4gIC8vIFVzZSBjb2RlYyBmcm9tIHBhcmFtcyBpZiBzcGVjaWZpZWQgdmlhIFVSTCBwYXJhbSwgb3RoZXJ3aXNlIHVzZSBmcm9tIFNEUC5cbiAgY29uc3QgY29kZWMgPSBwYXJhbXMudmlkZW9TZW5kQ29kZWMgfHwgY29kZWNOYW1lO1xuICBzZHAgPSBzZXRDb2RlY1BhcmFtKHNkcCwgY29kZWMsICd4LWdvb2dsZS1taW4tYml0cmF0ZScsXG4gICAgICBwYXJhbXMudmlkZW9TZW5kSW5pdGlhbEJpdHJhdGUudG9TdHJpbmcoKSk7XG4gIHNkcCA9IHNldENvZGVjUGFyYW0oc2RwLCBjb2RlYywgJ3gtZ29vZ2xlLW1heC1iaXRyYXRlJyxcbiAgICAgIG1heEJpdHJhdGUudG9TdHJpbmcoKSk7XG5cbiAgcmV0dXJuIHNkcDtcbn1cblxuZnVuY3Rpb24gcmVtb3ZlUGF5bG9hZFR5cGVGcm9tTWxpbmUobUxpbmUsIHBheWxvYWRUeXBlKSB7XG4gIG1MaW5lID0gbUxpbmUuc3BsaXQoJyAnKTtcbiAgZm9yIChsZXQgaSA9IDA7IGkgPCBtTGluZS5sZW5ndGg7ICsraSkge1xuICAgIGlmIChtTGluZVtpXSA9PT0gcGF5bG9hZFR5cGUudG9TdHJpbmcoKSkge1xuICAgICAgbUxpbmUuc3BsaWNlKGksIDEpO1xuICAgIH1cbiAgfVxuICByZXR1cm4gbUxpbmUuam9pbignICcpO1xufVxuXG5mdW5jdGlvbiByZW1vdmVDb2RlY0J5TmFtZShzZHBMaW5lcywgY29kZWMpIHtcbiAgY29uc3QgaW5kZXggPSBmaW5kTGluZShzZHBMaW5lcywgJ2E9cnRwbWFwJywgY29kZWMpO1xuICBpZiAoaW5kZXggPT09IG51bGwpIHtcbiAgICByZXR1cm4gc2RwTGluZXM7XG4gIH1cbiAgY29uc3QgcGF5bG9hZFR5cGUgPSBnZXRDb2RlY1BheWxvYWRUeXBlRnJvbUxpbmUoc2RwTGluZXNbaW5kZXhdKTtcbiAgc2RwTGluZXMuc3BsaWNlKGluZGV4LCAxKTtcblxuICAvLyBTZWFyY2ggZm9yIHRoZSB2aWRlbyBtPSBsaW5lIGFuZCByZW1vdmUgdGhlIGNvZGVjLlxuICBjb25zdCBtTGluZUluZGV4ID0gZmluZExpbmUoc2RwTGluZXMsICdtPScsICd2aWRlbycpO1xuICBpZiAobUxpbmVJbmRleCA9PT0gbnVsbCkge1xuICAgIHJldHVybiBzZHBMaW5lcztcbiAgfVxuICBzZHBMaW5lc1ttTGluZUluZGV4XSA9IHJlbW92ZVBheWxvYWRUeXBlRnJvbU1saW5lKHNkcExpbmVzW21MaW5lSW5kZXhdLFxuICAgICAgcGF5bG9hZFR5cGUpO1xuICByZXR1cm4gc2RwTGluZXM7XG59XG5cbmZ1bmN0aW9uIHJlbW92ZUNvZGVjQnlQYXlsb2FkVHlwZShzZHBMaW5lcywgcGF5bG9hZFR5cGUpIHtcbiAgY29uc3QgaW5kZXggPSBmaW5kTGluZShzZHBMaW5lcywgJ2E9cnRwbWFwJywgcGF5bG9hZFR5cGUudG9TdHJpbmcoKSk7XG4gIGlmIChpbmRleCA9PT0gbnVsbCkge1xuICAgIHJldHVybiBzZHBMaW5lcztcbiAgfVxuICBzZHBMaW5lcy5zcGxpY2UoaW5kZXgsIDEpO1xuXG4gIC8vIFNlYXJjaCBmb3IgdGhlIHZpZGVvIG09IGxpbmUgYW5kIHJlbW92ZSB0aGUgY29kZWMuXG4gIGNvbnN0IG1MaW5lSW5kZXggPSBmaW5kTGluZShzZHBMaW5lcywgJ209JywgJ3ZpZGVvJyk7XG4gIGlmIChtTGluZUluZGV4ID09PSBudWxsKSB7XG4gICAgcmV0dXJuIHNkcExpbmVzO1xuICB9XG4gIHNkcExpbmVzW21MaW5lSW5kZXhdID0gcmVtb3ZlUGF5bG9hZFR5cGVGcm9tTWxpbmUoc2RwTGluZXNbbUxpbmVJbmRleF0sXG4gICAgICBwYXlsb2FkVHlwZSk7XG4gIHJldHVybiBzZHBMaW5lcztcbn1cblxuZnVuY3Rpb24gbWF5YmVSZW1vdmVWaWRlb0ZlYyhzZHAsIHBhcmFtcykge1xuICBpZiAocGFyYW1zLnZpZGVvRmVjICE9PSAnZmFsc2UnKSB7XG4gICAgcmV0dXJuIHNkcDtcbiAgfVxuXG4gIGxldCBzZHBMaW5lcyA9IHNkcC5zcGxpdCgnXFxyXFxuJyk7XG5cbiAgbGV0IGluZGV4ID0gZmluZExpbmUoc2RwTGluZXMsICdhPXJ0cG1hcCcsICdyZWQnKTtcbiAgaWYgKGluZGV4ID09PSBudWxsKSB7XG4gICAgcmV0dXJuIHNkcDtcbiAgfVxuICBjb25zdCByZWRQYXlsb2FkVHlwZSA9IGdldENvZGVjUGF5bG9hZFR5cGVGcm9tTGluZShzZHBMaW5lc1tpbmRleF0pO1xuICBzZHBMaW5lcyA9IHJlbW92ZUNvZGVjQnlQYXlsb2FkVHlwZShzZHBMaW5lcywgcmVkUGF5bG9hZFR5cGUpO1xuXG4gIHNkcExpbmVzID0gcmVtb3ZlQ29kZWNCeU5hbWUoc2RwTGluZXMsICd1bHBmZWMnKTtcblxuICAvLyBSZW1vdmUgZm10cCBsaW5lcyBhc3NvY2lhdGVkIHdpdGggcmVkIGNvZGVjLlxuICBpbmRleCA9IGZpbmRMaW5lKHNkcExpbmVzLCAnYT1mbXRwJywgcmVkUGF5bG9hZFR5cGUudG9TdHJpbmcoKSk7XG4gIGlmIChpbmRleCA9PT0gbnVsbCkge1xuICAgIHJldHVybiBzZHA7XG4gIH1cbiAgY29uc3QgZm10cExpbmUgPSBwYXJzZUZtdHBMaW5lKHNkcExpbmVzW2luZGV4XSk7XG4gIGNvbnN0IHJ0eFBheWxvYWRUeXBlID0gZm10cExpbmUucHQ7XG4gIGlmIChydHhQYXlsb2FkVHlwZSA9PT0gbnVsbCkge1xuICAgIHJldHVybiBzZHA7XG4gIH1cbiAgc2RwTGluZXMuc3BsaWNlKGluZGV4LCAxKTtcblxuICBzZHBMaW5lcyA9IHJlbW92ZUNvZGVjQnlQYXlsb2FkVHlwZShzZHBMaW5lcywgcnR4UGF5bG9hZFR5cGUpO1xuICByZXR1cm4gc2RwTGluZXMuam9pbignXFxyXFxuJyk7XG59XG5cbi8vIFByb21vdGVzIHxhdWRpb1NlbmRDb2RlY3wgdG8gYmUgdGhlIGZpcnN0IGluIHRoZSBtPWF1ZGlvIGxpbmUsIGlmIHNldC5cbmZ1bmN0aW9uIG1heWJlUHJlZmVyQXVkaW9TZW5kQ29kZWMoc2RwLCBwYXJhbXMpIHtcbiAgcmV0dXJuIG1heWJlUHJlZmVyQ29kZWMoc2RwLCAnYXVkaW8nLCAnc2VuZCcsIHBhcmFtcy5hdWRpb1NlbmRDb2RlYyk7XG59XG5cbi8vIFByb21vdGVzIHxhdWRpb1JlY3ZDb2RlY3wgdG8gYmUgdGhlIGZpcnN0IGluIHRoZSBtPWF1ZGlvIGxpbmUsIGlmIHNldC5cbmZ1bmN0aW9uIG1heWJlUHJlZmVyQXVkaW9SZWNlaXZlQ29kZWMoc2RwLCBwYXJhbXMpIHtcbiAgcmV0dXJuIG1heWJlUHJlZmVyQ29kZWMoc2RwLCAnYXVkaW8nLCAncmVjZWl2ZScsIHBhcmFtcy5hdWRpb1JlY3ZDb2RlYyk7XG59XG5cbi8vIFByb21vdGVzIHx2aWRlb1NlbmRDb2RlY3wgdG8gYmUgdGhlIGZpcnN0IGluIHRoZSBtPWF1ZGlvIGxpbmUsIGlmIHNldC5cbmZ1bmN0aW9uIG1heWJlUHJlZmVyVmlkZW9TZW5kQ29kZWMoc2RwLCBwYXJhbXMpIHtcbiAgcmV0dXJuIG1heWJlUHJlZmVyQ29kZWMoc2RwLCAndmlkZW8nLCAnc2VuZCcsIHBhcmFtcy52aWRlb1NlbmRDb2RlYyk7XG59XG5cbi8vIFByb21vdGVzIHx2aWRlb1JlY3ZDb2RlY3wgdG8gYmUgdGhlIGZpcnN0IGluIHRoZSBtPWF1ZGlvIGxpbmUsIGlmIHNldC5cbmZ1bmN0aW9uIG1heWJlUHJlZmVyVmlkZW9SZWNlaXZlQ29kZWMoc2RwLCBwYXJhbXMpIHtcbiAgcmV0dXJuIG1heWJlUHJlZmVyQ29kZWMoc2RwLCAndmlkZW8nLCAncmVjZWl2ZScsIHBhcmFtcy52aWRlb1JlY3ZDb2RlYyk7XG59XG5cbi8vIFNldHMgfGNvZGVjfCBhcyB0aGUgZGVmYXVsdCB8dHlwZXwgY29kZWMgaWYgaXQncyBwcmVzZW50LlxuLy8gVGhlIGZvcm1hdCBvZiB8Y29kZWN8IGlzICdOQU1FL1JBVEUnLCBlLmcuICdvcHVzLzQ4MDAwJy5cbmZ1bmN0aW9uIG1heWJlUHJlZmVyQ29kZWMoc2RwLCB0eXBlLCBkaXIsIGNvZGVjKSB7XG4gIGNvbnN0IHN0ciA9IHR5cGUgKyAnICcgKyBkaXIgKyAnIGNvZGVjJztcbiAgaWYgKCFjb2RlYykge1xuICAgIExvZ2dlci5kZWJ1ZygnTm8gcHJlZmVyZW5jZSBvbiAnICsgc3RyICsgJy4nKTtcbiAgICByZXR1cm4gc2RwO1xuICB9XG5cbiAgTG9nZ2VyLmRlYnVnKCdQcmVmZXIgJyArIHN0ciArICc6ICcgKyBjb2RlYyk7XG5cbiAgY29uc3Qgc2RwTGluZXMgPSBzZHAuc3BsaXQoJ1xcclxcbicpO1xuXG4gIC8vIFNlYXJjaCBmb3IgbSBsaW5lLlxuICBjb25zdCBtTGluZUluZGV4ID0gZmluZExpbmUoc2RwTGluZXMsICdtPScsIHR5cGUpO1xuICBpZiAobUxpbmVJbmRleCA9PT0gbnVsbCkge1xuICAgIHJldHVybiBzZHA7XG4gIH1cblxuICAvLyBJZiB0aGUgY29kZWMgaXMgYXZhaWxhYmxlLCBzZXQgaXQgYXMgdGhlIGRlZmF1bHQgaW4gbSBsaW5lLlxuICBsZXQgcGF5bG9hZCA9IG51bGw7XG4gIGZvciAobGV0IGkgPSAwOyBpIDwgc2RwTGluZXMubGVuZ3RoOyBpKyspIHtcbiAgICBjb25zdCBpbmRleCA9IGZpbmRMaW5lSW5SYW5nZShzZHBMaW5lcywgaSwgLTEsICdhPXJ0cG1hcCcsIGNvZGVjKTtcbiAgICBpZiAoaW5kZXggIT09IG51bGwpIHtcbiAgICAgIHBheWxvYWQgPSBnZXRDb2RlY1BheWxvYWRUeXBlRnJvbUxpbmUoc2RwTGluZXNbaW5kZXhdKTtcbiAgICAgIGlmIChwYXlsb2FkKSB7XG4gICAgICAgIHNkcExpbmVzW21MaW5lSW5kZXhdID0gc2V0RGVmYXVsdENvZGVjKHNkcExpbmVzW21MaW5lSW5kZXhdLCBwYXlsb2FkKTtcbiAgICAgIH1cbiAgICB9XG4gIH1cblxuICBzZHAgPSBzZHBMaW5lcy5qb2luKCdcXHJcXG4nKTtcbiAgcmV0dXJuIHNkcDtcbn1cblxuLy8gU2V0IGZtdHAgcGFyYW0gdG8gc3BlY2lmaWMgY29kZWMgaW4gU0RQLiBJZiBwYXJhbSBkb2VzIG5vdCBleGlzdHMsIGFkZCBpdC5cbmZ1bmN0aW9uIHNldENvZGVjUGFyYW0oc2RwLCBjb2RlYywgcGFyYW0sIHZhbHVlLCBtaWQpIHtcbiAgbGV0IHNkcExpbmVzID0gc2RwLnNwbGl0KCdcXHJcXG4nKTtcbiAgbGV0IGhlYWRMaW5lcyA9IG51bGw7XG4gIGxldCB0YWlsTGluZXMgPSBudWxsO1xuICBpZiAodHlwZW9mIG1pZCA9PT0gJ3N0cmluZycpIHtcbiAgICBjb25zdCBtaWRSYW5nZSA9IGZpbmRNTGluZVJhbmdlV2l0aE1JRChzZHBMaW5lcywgbWlkKTtcbiAgICBpZiAobWlkUmFuZ2UpIHtcbiAgICAgIGNvbnN0IHsgc3RhcnQsIGVuZCB9ID0gbWlkUmFuZ2U7XG4gICAgICBoZWFkTGluZXMgPSBzZHBMaW5lcy5zbGljZSgwLCBzdGFydCk7XG4gICAgICB0YWlsTGluZXMgPSBzZHBMaW5lcy5zbGljZShlbmQpO1xuICAgICAgc2RwTGluZXMgPSBzZHBMaW5lcy5zbGljZShzdGFydCwgZW5kKTtcbiAgICB9XG4gIH1cblxuICAvLyBTRFBzIHNlbnQgZnJvbSBNQ1UgdXNlIFxcbiBhcyBsaW5lIGJyZWFrLlxuICBpZiAoc2RwTGluZXMubGVuZ3RoIDw9IDEpIHtcbiAgICBzZHBMaW5lcyA9IHNkcC5zcGxpdCgnXFxuJyk7XG4gIH1cblxuICBjb25zdCBmbXRwTGluZUluZGV4ID0gZmluZEZtdHBMaW5lKHNkcExpbmVzLCBjb2RlYyk7XG5cbiAgbGV0IGZtdHBPYmogPSB7fTtcbiAgaWYgKGZtdHBMaW5lSW5kZXggPT09IG51bGwpIHtcbiAgICBjb25zdCBpbmRleCA9IGZpbmRMaW5lKHNkcExpbmVzLCAnYT1ydHBtYXAnLCBjb2RlYyk7XG4gICAgaWYgKGluZGV4ID09PSBudWxsKSB7XG4gICAgICByZXR1cm4gc2RwO1xuICAgIH1cbiAgICBjb25zdCBwYXlsb2FkID0gZ2V0Q29kZWNQYXlsb2FkVHlwZUZyb21MaW5lKHNkcExpbmVzW2luZGV4XSk7XG4gICAgZm10cE9iai5wdCA9IHBheWxvYWQudG9TdHJpbmcoKTtcbiAgICBmbXRwT2JqLnBhcmFtcyA9IHt9O1xuICAgIGZtdHBPYmoucGFyYW1zW3BhcmFtXSA9IHZhbHVlO1xuICAgIHNkcExpbmVzLnNwbGljZShpbmRleCArIDEsIDAsIHdyaXRlRm10cExpbmUoZm10cE9iaikpO1xuICB9IGVsc2Uge1xuICAgIGZtdHBPYmogPSBwYXJzZUZtdHBMaW5lKHNkcExpbmVzW2ZtdHBMaW5lSW5kZXhdKTtcbiAgICBmbXRwT2JqLnBhcmFtc1twYXJhbV0gPSB2YWx1ZTtcbiAgICBzZHBMaW5lc1tmbXRwTGluZUluZGV4XSA9IHdyaXRlRm10cExpbmUoZm10cE9iaik7XG4gIH1cblxuICBpZiAoaGVhZExpbmVzKSB7XG4gICAgc2RwTGluZXMgPSBoZWFkTGluZXMuY29uY2F0KHNkcExpbmVzKS5jb25jYXQodGFpbExpbmVzKTtcbiAgfVxuICBzZHAgPSBzZHBMaW5lcy5qb2luKCdcXHJcXG4nKTtcbiAgcmV0dXJuIHNkcDtcbn1cblxuLy8gUmVtb3ZlIGZtdHAgcGFyYW0gaWYgaXQgZXhpc3RzLlxuZnVuY3Rpb24gcmVtb3ZlQ29kZWNQYXJhbShzZHAsIGNvZGVjLCBwYXJhbSkge1xuICBjb25zdCBzZHBMaW5lcyA9IHNkcC5zcGxpdCgnXFxyXFxuJyk7XG5cbiAgY29uc3QgZm10cExpbmVJbmRleCA9IGZpbmRGbXRwTGluZShzZHBMaW5lcywgY29kZWMpO1xuICBpZiAoZm10cExpbmVJbmRleCA9PT0gbnVsbCkge1xuICAgIHJldHVybiBzZHA7XG4gIH1cblxuICBjb25zdCBtYXAgPSBwYXJzZUZtdHBMaW5lKHNkcExpbmVzW2ZtdHBMaW5lSW5kZXhdKTtcbiAgZGVsZXRlIG1hcC5wYXJhbXNbcGFyYW1dO1xuXG4gIGNvbnN0IG5ld0xpbmUgPSB3cml0ZUZtdHBMaW5lKG1hcCk7XG4gIGlmIChuZXdMaW5lID09PSBudWxsKSB7XG4gICAgc2RwTGluZXMuc3BsaWNlKGZtdHBMaW5lSW5kZXgsIDEpO1xuICB9IGVsc2Uge1xuICAgIHNkcExpbmVzW2ZtdHBMaW5lSW5kZXhdID0gbmV3TGluZTtcbiAgfVxuXG4gIHNkcCA9IHNkcExpbmVzLmpvaW4oJ1xcclxcbicpO1xuICByZXR1cm4gc2RwO1xufVxuXG4vLyBTcGxpdCBhbiBmbXRwIGxpbmUgaW50byBhbiBvYmplY3QgaW5jbHVkaW5nICdwdCcgYW5kICdwYXJhbXMnLlxuZnVuY3Rpb24gcGFyc2VGbXRwTGluZShmbXRwTGluZSkge1xuICBjb25zdCBmbXRwT2JqID0ge307XG4gIGNvbnN0IHNwYWNlUG9zID0gZm10cExpbmUuaW5kZXhPZignICcpO1xuICBjb25zdCBrZXlWYWx1ZXMgPSBmbXRwTGluZS5zdWJzdHJpbmcoc3BhY2VQb3MgKyAxKS5zcGxpdCgnOycpO1xuXG4gIGNvbnN0IHBhdHRlcm4gPSBuZXcgUmVnRXhwKCdhPWZtdHA6KFxcXFxkKyknKTtcbiAgY29uc3QgcmVzdWx0ID0gZm10cExpbmUubWF0Y2gocGF0dGVybik7XG4gIGlmIChyZXN1bHQgJiYgcmVzdWx0Lmxlbmd0aCA9PT0gMikge1xuICAgIGZtdHBPYmoucHQgPSByZXN1bHRbMV07XG4gIH0gZWxzZSB7XG4gICAgcmV0dXJuIG51bGw7XG4gIH1cblxuICBjb25zdCBwYXJhbXMgPSB7fTtcbiAgZm9yIChsZXQgaSA9IDA7IGkgPCBrZXlWYWx1ZXMubGVuZ3RoOyArK2kpIHtcbiAgICBjb25zdCBwYWlyID0ga2V5VmFsdWVzW2ldLnNwbGl0KCc9Jyk7XG4gICAgaWYgKHBhaXIubGVuZ3RoID09PSAyKSB7XG4gICAgICBwYXJhbXNbcGFpclswXV0gPSBwYWlyWzFdO1xuICAgIH1cbiAgfVxuICBmbXRwT2JqLnBhcmFtcyA9IHBhcmFtcztcblxuICByZXR1cm4gZm10cE9iajtcbn1cblxuLy8gR2VuZXJhdGUgYW4gZm10cCBsaW5lIGZyb20gYW4gb2JqZWN0IGluY2x1ZGluZyAncHQnIGFuZCAncGFyYW1zJy5cbmZ1bmN0aW9uIHdyaXRlRm10cExpbmUoZm10cE9iaikge1xuICBpZiAoIWZtdHBPYmouaGFzT3duUHJvcGVydHkoJ3B0JykgfHwgIWZtdHBPYmouaGFzT3duUHJvcGVydHkoJ3BhcmFtcycpKSB7XG4gICAgcmV0dXJuIG51bGw7XG4gIH1cbiAgY29uc3QgcHQgPSBmbXRwT2JqLnB0O1xuICBjb25zdCBwYXJhbXMgPSBmbXRwT2JqLnBhcmFtcztcbiAgY29uc3Qga2V5VmFsdWVzID0gW107XG4gIGxldCBpID0gMDtcbiAgZm9yIChjb25zdCBrZXkgaW4gcGFyYW1zKSB7XG4gICAga2V5VmFsdWVzW2ldID0ga2V5ICsgJz0nICsgcGFyYW1zW2tleV07XG4gICAgKytpO1xuICB9XG4gIGlmIChpID09PSAwKSB7XG4gICAgcmV0dXJuIG51bGw7XG4gIH1cbiAgcmV0dXJuICdhPWZtdHA6JyArIHB0LnRvU3RyaW5nKCkgKyAnICcgKyBrZXlWYWx1ZXMuam9pbignOycpO1xufVxuXG4vLyBGaW5kIGZtdHAgYXR0cmlidXRlIGZvciB8Y29kZWN8IGluIHxzZHBMaW5lc3wuXG5mdW5jdGlvbiBmaW5kRm10cExpbmUoc2RwTGluZXMsIGNvZGVjKSB7XG4gIC8vIEZpbmQgcGF5bG9hZCBvZiBjb2RlYy5cbiAgY29uc3QgcGF5bG9hZCA9IGdldENvZGVjUGF5bG9hZFR5cGUoc2RwTGluZXMsIGNvZGVjKTtcbiAgLy8gRmluZCB0aGUgcGF5bG9hZCBpbiBmbXRwIGxpbmUuXG4gIHJldHVybiBwYXlsb2FkID8gZmluZExpbmUoc2RwTGluZXMsICdhPWZtdHA6JyArIHBheWxvYWQudG9TdHJpbmcoKSkgOiBudWxsO1xufVxuXG4vLyBGaW5kIHRoZSBsaW5lIGluIHNkcExpbmVzIHRoYXQgc3RhcnRzIHdpdGggfHByZWZpeHwsIGFuZCwgaWYgc3BlY2lmaWVkLFxuLy8gY29udGFpbnMgfHN1YnN0cnwgKGNhc2UtaW5zZW5zaXRpdmUgc2VhcmNoKS5cbmZ1bmN0aW9uIGZpbmRMaW5lKHNkcExpbmVzLCBwcmVmaXgsIHN1YnN0cikge1xuICByZXR1cm4gZmluZExpbmVJblJhbmdlKHNkcExpbmVzLCAwLCAtMSwgcHJlZml4LCBzdWJzdHIpO1xufVxuXG4vLyBGaW5kIHRoZSBsaW5lIGluIHNkcExpbmVzW3N0YXJ0TGluZS4uLmVuZExpbmUgLSAxXSB0aGF0IHN0YXJ0cyB3aXRoIHxwcmVmaXh8XG4vLyBhbmQsIGlmIHNwZWNpZmllZCwgY29udGFpbnMgfHN1YnN0cnwgKGNhc2UtaW5zZW5zaXRpdmUgc2VhcmNoKS5cbmZ1bmN0aW9uIGZpbmRMaW5lSW5SYW5nZShzZHBMaW5lcywgc3RhcnRMaW5lLCBlbmRMaW5lLCBwcmVmaXgsIHN1YnN0cikge1xuICBjb25zdCByZWFsRW5kTGluZSA9IGVuZExpbmUgIT09IC0xID8gZW5kTGluZSA6IHNkcExpbmVzLmxlbmd0aDtcbiAgZm9yIChsZXQgaSA9IHN0YXJ0TGluZTsgaSA8IHJlYWxFbmRMaW5lOyArK2kpIHtcbiAgICBpZiAoc2RwTGluZXNbaV0uaW5kZXhPZihwcmVmaXgpID09PSAwKSB7XG4gICAgICBpZiAoIXN1YnN0ciB8fFxuICAgICAgICAgIHNkcExpbmVzW2ldLnRvTG93ZXJDYXNlKCkuaW5kZXhPZihzdWJzdHIudG9Mb3dlckNhc2UoKSkgIT09IC0xKSB7XG4gICAgICAgIHJldHVybiBpO1xuICAgICAgfVxuICAgIH1cbiAgfVxuICByZXR1cm4gbnVsbDtcbn1cblxuLy8gR2V0cyB0aGUgY29kZWMgcGF5bG9hZCB0eXBlIGZyb20gc2RwIGxpbmVzLlxuZnVuY3Rpb24gZ2V0Q29kZWNQYXlsb2FkVHlwZShzZHBMaW5lcywgY29kZWMpIHtcbiAgY29uc3QgaW5kZXggPSBmaW5kTGluZShzZHBMaW5lcywgJ2E9cnRwbWFwJywgY29kZWMpO1xuICByZXR1cm4gaW5kZXggPyBnZXRDb2RlY1BheWxvYWRUeXBlRnJvbUxpbmUoc2RwTGluZXNbaW5kZXhdKSA6IG51bGw7XG59XG5cbi8vIEdldHMgdGhlIGNvZGVjIHBheWxvYWQgdHlwZSBmcm9tIGFuIGE9cnRwbWFwOlggbGluZS5cbmZ1bmN0aW9uIGdldENvZGVjUGF5bG9hZFR5cGVGcm9tTGluZShzZHBMaW5lKSB7XG4gIGNvbnN0IHBhdHRlcm4gPSBuZXcgUmVnRXhwKCdhPXJ0cG1hcDooXFxcXGQrKSBbYS16QS1aMC05LV0rXFxcXC9cXFxcZCsnKTtcbiAgY29uc3QgcmVzdWx0ID0gc2RwTGluZS5tYXRjaChwYXR0ZXJuKTtcbiAgcmV0dXJuIChyZXN1bHQgJiYgcmVzdWx0Lmxlbmd0aCA9PT0gMikgPyByZXN1bHRbMV0gOiBudWxsO1xufVxuXG4vLyBSZXR1cm5zIGEgbmV3IG09IGxpbmUgd2l0aCB0aGUgc3BlY2lmaWVkIGNvZGVjIGFzIHRoZSBmaXJzdCBvbmUuXG5mdW5jdGlvbiBzZXREZWZhdWx0Q29kZWMobUxpbmUsIHBheWxvYWQpIHtcbiAgY29uc3QgZWxlbWVudHMgPSBtTGluZS5zcGxpdCgnICcpO1xuXG4gIC8vIEp1c3QgY29weSB0aGUgZmlyc3QgdGhyZWUgcGFyYW1ldGVyczsgY29kZWMgb3JkZXIgc3RhcnRzIG9uIGZvdXJ0aC5cbiAgY29uc3QgbmV3TGluZSA9IGVsZW1lbnRzLnNsaWNlKDAsIDMpO1xuXG4gIC8vIFB1dCB0YXJnZXQgcGF5bG9hZCBmaXJzdCBhbmQgY29weSBpbiB0aGUgcmVzdC5cbiAgbmV3TGluZS5wdXNoKHBheWxvYWQpO1xuICBmb3IgKGxldCBpID0gMzsgaSA8IGVsZW1lbnRzLmxlbmd0aDsgaSsrKSB7XG4gICAgaWYgKGVsZW1lbnRzW2ldICE9PSBwYXlsb2FkKSB7XG4gICAgICBuZXdMaW5lLnB1c2goZWxlbWVudHNbaV0pO1xuICAgIH1cbiAgfVxuICByZXR1cm4gbmV3TGluZS5qb2luKCcgJyk7XG59XG5cbi8qIEJlbG93IGFyZSBuZXdseSBhZGRlZCBmdW5jdGlvbnMgKi9cblxuLy8gRm9sbG93aW5nIGNvZGVjcyB3aWxsIG5vdCBiZSByZW1vdmVkIGZyb20gU0RQIGV2ZW50IHRoZXkgYXJlIG5vdCBpbiB0aGVcbi8vIHVzZXItc3BlY2lmaWVkIGNvZGVjIGxpc3QuXG5jb25zdCBhdWRpb0NvZGVjQWxsb3dMaXN0ID0gWydDTicsICd0ZWxlcGhvbmUtZXZlbnQnXTtcbmNvbnN0IHZpZGVvQ29kZWNBbGxvd0xpc3QgPSBbJ3JlZCcsICd1bHBmZWMnLCAnZmxleGZlYyddO1xuXG4vLyBSZXR1cm5zIGEgbmV3IG09IGxpbmUgd2l0aCB0aGUgc3BlY2lmaWVkIGNvZGVjIG9yZGVyLlxuZnVuY3Rpb24gc2V0Q29kZWNPcmRlcihtTGluZSwgcGF5bG9hZHMpIHtcbiAgY29uc3QgZWxlbWVudHMgPSBtTGluZS5zcGxpdCgnICcpO1xuXG4gIC8vIEp1c3QgY29weSB0aGUgZmlyc3QgdGhyZWUgcGFyYW1ldGVyczsgY29kZWMgb3JkZXIgc3RhcnRzIG9uIGZvdXJ0aC5cbiAgbGV0IG5ld0xpbmUgPSBlbGVtZW50cy5zbGljZSgwLCAzKTtcblxuICAvLyBDb25jYXQgcGF5bG9hZCB0eXBlcy5cbiAgbmV3TGluZSA9IG5ld0xpbmUuY29uY2F0KHBheWxvYWRzKTtcblxuICByZXR1cm4gbmV3TGluZS5qb2luKCcgJyk7XG59XG5cbi8vIEFwcGVuZCBSVFggcGF5bG9hZHMgZm9yIGV4aXN0aW5nIHBheWxvYWRzLlxuZnVuY3Rpb24gYXBwZW5kUnR4UGF5bG9hZHMoc2RwTGluZXMsIHBheWxvYWRzKSB7XG4gIGZvciAoY29uc3QgcGF5bG9hZCBvZiBwYXlsb2Fkcykge1xuICAgIGNvbnN0IGluZGV4ID0gZmluZExpbmUoc2RwTGluZXMsICdhPWZtdHAnLCAnYXB0PScgKyBwYXlsb2FkKTtcbiAgICBpZiAoaW5kZXggIT09IG51bGwpIHtcbiAgICAgIGNvbnN0IGZtdHBMaW5lID0gcGFyc2VGbXRwTGluZShzZHBMaW5lc1tpbmRleF0pO1xuICAgICAgcGF5bG9hZHMucHVzaChmbXRwTGluZS5wdCk7XG4gICAgfVxuICB9XG4gIHJldHVybiBwYXlsb2Fkcztcbn1cblxuLy8gUmVtb3ZlIGEgY29kZWMgd2l0aCBhbGwgaXRzIGFzc29jaWF0ZWQgYSBsaW5lcy5cbmZ1bmN0aW9uIHJlbW92ZUNvZGVjRnJhbUFMaW5lKHNkcExpbmVzLCBwYXlsb2FkKSB7XG4gIGNvbnN0IHBhdHRlcm4gPSBuZXcgUmVnRXhwKCdhPShydHBtYXB8cnRjcC1mYnxmbXRwKTonK3BheWxvYWQrJ1xcXFxzJyk7XG4gIGZvciAobGV0IGk9c2RwTGluZXMubGVuZ3RoLTE7IGk+MDsgaS0tKSB7XG4gICAgaWYgKHNkcExpbmVzW2ldLm1hdGNoKHBhdHRlcm4pKSB7XG4gICAgICBzZHBMaW5lcy5zcGxpY2UoaSwgMSk7XG4gICAgfVxuICB9XG4gIHJldHVybiBzZHBMaW5lcztcbn1cblxuLy8gRmluZCBtLWxpbmUgYW5kIG5leHQgbS1saW5lIHdpdGggZ2l2ZSBtaWQsIHJldHVybiB7IHN0YXJ0LCBlbmQgfS5cbmZ1bmN0aW9uIGZpbmRNTGluZVJhbmdlV2l0aE1JRChzZHBMaW5lcywgbWlkKSB7XG4gIGNvbnN0IG1pZExpbmUgPSAnYT1taWQ6JyArIG1pZDtcbiAgbGV0IG1pZEluZGV4ID0gZmluZExpbmUoc2RwTGluZXMsIG1pZExpbmUpO1xuICAvLyBDb21wYXJlIHRoZSB3aG9sZSBsaW5lIHNpbmNlIGZpbmRMaW5lIG9ubHkgY29tcGFyZXMgcHJlZml4XG4gIHdoaWxlIChtaWRJbmRleCA+PSAwICYmIHNkcExpbmVzW21pZEluZGV4XSAhPT0gbWlkTGluZSkge1xuICAgIG1pZEluZGV4ID0gZmluZExpbmVJblJhbmdlKHNkcExpbmVzLCBtaWRJbmRleCwgLTEsIG1pZExpbmUpO1xuICB9XG4gIGlmIChtaWRJbmRleCA+PSAwKSB7XG4gICAgLy8gRm91bmQgbWF0Y2hlZCBhPW1pZCBsaW5lXG4gICAgY29uc3QgbmV4dE1MaW5lSW5kZXggPSAoZmluZExpbmVJblJhbmdlKHNkcExpbmVzLCBtaWRJbmRleCwgLTEsICdtPScpXG4gICAgICAgIHx8IC0xKTtcbiAgICBsZXQgbUxpbmVJbmRleCA9IC0xO1xuICAgIGZvciAobGV0IGkgPSBtaWRJbmRleDsgaSA+PSAwOyBpLS0pIHtcbiAgICAgIGlmIChzZHBMaW5lc1tpXS5pbmRleE9mKCdtPScpID49IDApIHtcbiAgICAgICAgbUxpbmVJbmRleCA9IGk7XG4gICAgICAgIGJyZWFrO1xuICAgICAgfVxuICAgIH1cbiAgICBpZiAobUxpbmVJbmRleCA+PSAwKSB7XG4gICAgICByZXR1cm4ge1xuICAgICAgICBzdGFydDogbUxpbmVJbmRleCxcbiAgICAgICAgZW5kOiBuZXh0TUxpbmVJbmRleFxuICAgICAgfTtcbiAgICB9XG4gIH1cbiAgcmV0dXJuIG51bGw7XG59XG5cbi8vIFJlb3JkZXIgY29kZWNzIGluIG0tbGluZSBhY2NvcmRpbmcgdGhlIG9yZGVyIG9mIHxjb2RlY3N8LiBSZW1vdmUgY29kZWNzIGZyb21cbi8vIG0tbGluZSBpZiBpdCBpcyBub3QgcHJlc2VudCBpbiB8Y29kZWNzfFxuLy8gQXBwbGllZCBvbiBzcGVjaWZpYyBtLWxpbmUgaWYgbWlkIGlzIHByZXNlbnRlZFxuLy8gVGhlIGZvcm1hdCBvZiB8Y29kZWN8IGlzICdOQU1FL1JBVEUnLCBlLmcuICdvcHVzLzQ4MDAwJy5cbmV4cG9ydCBmdW5jdGlvbiByZW9yZGVyQ29kZWNzKHNkcCwgdHlwZSwgY29kZWNzLCBtaWQpIHtcbiAgaWYgKCFjb2RlY3MgfHwgY29kZWNzLmxlbmd0aCA9PT0gMCkge1xuICAgIHJldHVybiBzZHA7XG4gIH1cblxuICBjb2RlY3MgPSB0eXBlID09PSAnYXVkaW8nID8gY29kZWNzLmNvbmNhdChhdWRpb0NvZGVjQWxsb3dMaXN0KSA6IGNvZGVjcy5jb25jYXQoXG4gICAgICB2aWRlb0NvZGVjQWxsb3dMaXN0KTtcblxuICBsZXQgc2RwTGluZXMgPSBzZHAuc3BsaXQoJ1xcclxcbicpO1xuICBsZXQgaGVhZExpbmVzID0gbnVsbDtcbiAgbGV0IHRhaWxMaW5lcyA9IG51bGw7XG4gIGlmICh0eXBlb2YgbWlkID09PSAnc3RyaW5nJykge1xuICAgIGNvbnN0IG1pZFJhbmdlID0gZmluZE1MaW5lUmFuZ2VXaXRoTUlEKHNkcExpbmVzLCBtaWQpO1xuICAgIGlmIChtaWRSYW5nZSkge1xuICAgICAgY29uc3QgeyBzdGFydCwgZW5kIH0gPSBtaWRSYW5nZTtcbiAgICAgIGhlYWRMaW5lcyA9IHNkcExpbmVzLnNsaWNlKDAsIHN0YXJ0KTtcbiAgICAgIHRhaWxMaW5lcyA9IHNkcExpbmVzLnNsaWNlKGVuZCk7XG4gICAgICBzZHBMaW5lcyA9IHNkcExpbmVzLnNsaWNlKHN0YXJ0LCBlbmQpO1xuICAgIH1cbiAgfVxuXG4gIC8vIFNlYXJjaCBmb3IgbSBsaW5lLlxuICBjb25zdCBtTGluZUluZGV4ID0gZmluZExpbmUoc2RwTGluZXMsICdtPScsIHR5cGUpO1xuICBpZiAobUxpbmVJbmRleCA9PT0gbnVsbCkge1xuICAgIHJldHVybiBzZHA7XG4gIH1cblxuICBjb25zdCBvcmlnaW5QYXlsb2FkcyA9IHNkcExpbmVzW21MaW5lSW5kZXhdLnNwbGl0KCcgJyk7XG4gIG9yaWdpblBheWxvYWRzLnNwbGljZSgwLCAzKTtcblxuICAvLyBJZiB0aGUgY29kZWMgaXMgYXZhaWxhYmxlLCBzZXQgaXQgYXMgdGhlIGRlZmF1bHQgaW4gbSBsaW5lLlxuICBsZXQgcGF5bG9hZHMgPSBbXTtcbiAgZm9yIChjb25zdCBjb2RlYyBvZiBjb2RlY3MpIHtcbiAgICBmb3IgKGxldCBpID0gMDsgaSA8IHNkcExpbmVzLmxlbmd0aDsgaSsrKSB7XG4gICAgICBjb25zdCBpbmRleCA9IGZpbmRMaW5lSW5SYW5nZShzZHBMaW5lcywgaSwgLTEsICdhPXJ0cG1hcCcsIGNvZGVjKTtcbiAgICAgIGlmIChpbmRleCAhPT0gbnVsbCkge1xuICAgICAgICBjb25zdCBwYXlsb2FkID0gZ2V0Q29kZWNQYXlsb2FkVHlwZUZyb21MaW5lKHNkcExpbmVzW2luZGV4XSk7XG4gICAgICAgIGlmIChwYXlsb2FkKSB7XG4gICAgICAgICAgcGF5bG9hZHMucHVzaChwYXlsb2FkKTtcbiAgICAgICAgICBpID0gaW5kZXg7XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICB9XG4gIH1cbiAgcGF5bG9hZHMgPSBhcHBlbmRSdHhQYXlsb2FkcyhzZHBMaW5lcywgcGF5bG9hZHMpO1xuICBzZHBMaW5lc1ttTGluZUluZGV4XSA9IHNldENvZGVjT3JkZXIoc2RwTGluZXNbbUxpbmVJbmRleF0sIHBheWxvYWRzKTtcblxuICAvLyBSZW1vdmUgYSBsaW5lcy5cbiAgZm9yIChjb25zdCBwYXlsb2FkIG9mIG9yaWdpblBheWxvYWRzKSB7XG4gICAgaWYgKHBheWxvYWRzLmluZGV4T2YocGF5bG9hZCk9PT0tMSkge1xuICAgICAgc2RwTGluZXMgPSByZW1vdmVDb2RlY0ZyYW1BTGluZShzZHBMaW5lcywgcGF5bG9hZCk7XG4gICAgfVxuICB9XG5cbiAgaWYgKGhlYWRMaW5lcykge1xuICAgIHNkcExpbmVzID0gaGVhZExpbmVzLmNvbmNhdChzZHBMaW5lcykuY29uY2F0KHRhaWxMaW5lcyk7XG4gIH1cbiAgc2RwID0gc2RwTGluZXMuam9pbignXFxyXFxuJyk7XG4gIHJldHVybiBzZHA7XG59XG5cbi8vIEFkZCBsZWdhY3kgc2ltdWxjYXN0LlxuZXhwb3J0IGZ1bmN0aW9uIGFkZExlZ2FjeVNpbXVsY2FzdChzZHAsIHR5cGUsIG51bVN0cmVhbXMsIG1pZCkge1xuICBpZiAoIW51bVN0cmVhbXMgfHwgIShudW1TdHJlYW1zID4gMSkpIHtcbiAgICByZXR1cm4gc2RwO1xuICB9XG5cbiAgbGV0IHNkcExpbmVzID0gc2RwLnNwbGl0KCdcXHJcXG4nKTtcbiAgbGV0IGhlYWRMaW5lcyA9IG51bGw7XG4gIGxldCB0YWlsTGluZXMgPSBudWxsO1xuICBpZiAodHlwZW9mIG1pZCA9PT0gJ3N0cmluZycpIHtcbiAgICBjb25zdCBtaWRSYW5nZSA9IGZpbmRNTGluZVJhbmdlV2l0aE1JRChzZHBMaW5lcywgbWlkKTtcbiAgICBpZiAobWlkUmFuZ2UpIHtcbiAgICAgIGNvbnN0IHsgc3RhcnQsIGVuZCB9ID0gbWlkUmFuZ2U7XG4gICAgICBoZWFkTGluZXMgPSBzZHBMaW5lcy5zbGljZSgwLCBzdGFydCk7XG4gICAgICB0YWlsTGluZXMgPSBzZHBMaW5lcy5zbGljZShlbmQpO1xuICAgICAgc2RwTGluZXMgPSBzZHBMaW5lcy5zbGljZShzdGFydCwgZW5kKTtcbiAgICB9XG4gIH1cblxuICAvLyBTZWFyY2ggZm9yIG0gbGluZS5cbiAgY29uc3QgbUxpbmVTdGFydCA9IGZpbmRMaW5lKHNkcExpbmVzLCAnbT0nLCB0eXBlKTtcbiAgaWYgKG1MaW5lU3RhcnQgPT09IG51bGwpIHtcbiAgICByZXR1cm4gc2RwO1xuICB9XG4gIGxldCBtTGluZUVuZCA9IGZpbmRMaW5lSW5SYW5nZShzZHBMaW5lcywgbUxpbmVTdGFydCArIDEsIC0xLCAnbT0nKTtcbiAgaWYgKG1MaW5lRW5kID09PSBudWxsKSB7XG4gICAgbUxpbmVFbmQgPSBzZHBMaW5lcy5sZW5ndGg7XG4gIH1cblxuICBjb25zdCBzc3JjR2V0dGVyID0gKGxpbmUpID0+IHtcbiAgICBjb25zdCBwYXJ0cyA9IGxpbmUuc3BsaXQoJyAnKTtcbiAgICBjb25zdCBzc3JjID0gcGFydHNbMF0uc3BsaXQoJzonKVsxXTtcbiAgICByZXR1cm4gc3NyYztcbiAgfTtcblxuICAvLyBQcm9jZXNzIHNzcmMgbGluZXMgZnJvbSBtTGluZUluZGV4LlxuICBjb25zdCByZW1vdmVzID0gbmV3IFNldCgpO1xuICBjb25zdCBzc3JjcyA9IG5ldyBTZXQoKTtcbiAgY29uc3QgZ3NzcmNzID0gbmV3IFNldCgpO1xuICBjb25zdCBzaW1MaW5lcyA9IFtdO1xuICBjb25zdCBzaW1Hcm91cExpbmVzID0gW107XG4gIGxldCBpID0gbUxpbmVTdGFydCArIDE7XG4gIHdoaWxlIChpIDwgbUxpbmVFbmQpIHtcbiAgICBjb25zdCBsaW5lID0gc2RwTGluZXNbaV07XG4gICAgaWYgKGxpbmUgPT09ICcnKSB7XG4gICAgICBicmVhaztcbiAgICB9XG4gICAgaWYgKGxpbmUuaW5kZXhPZignYT1zc3JjOicpID4gLTEpIHtcbiAgICAgIGNvbnN0IHNzcmMgPSBzc3JjR2V0dGVyKHNkcExpbmVzW2ldKTtcbiAgICAgIHNzcmNzLmFkZChzc3JjKTtcbiAgICAgIGlmIChsaW5lLmluZGV4T2YoJ2NuYW1lJykgPiAtMSB8fCBsaW5lLmluZGV4T2YoJ21zaWQnKSA+IC0xKSB7XG4gICAgICAgIGZvciAobGV0IGogPSAxOyBqIDwgbnVtU3RyZWFtczsgaisrKSB7XG4gICAgICAgICAgY29uc3QgbnNzcmMgPSAocGFyc2VJbnQoc3NyYykgKyBqKSArICcnO1xuICAgICAgICAgIHNpbUxpbmVzLnB1c2gobGluZS5yZXBsYWNlKHNzcmMsIG5zc3JjKSk7XG4gICAgICAgIH1cbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIHJlbW92ZXMuYWRkKGxpbmUpO1xuICAgICAgfVxuICAgIH1cbiAgICBpZiAobGluZS5pbmRleE9mKCdhPXNzcmMtZ3JvdXA6RklEJykgPiAtMSkge1xuICAgICAgY29uc3QgcGFydHMgPSBsaW5lLnNwbGl0KCcgJyk7XG4gICAgICBnc3NyY3MuYWRkKHBhcnRzWzJdKTtcbiAgICAgIGZvciAobGV0IGogPSAxOyBqIDwgbnVtU3RyZWFtczsgaisrKSB7XG4gICAgICAgIGNvbnN0IG5zc3JjMSA9IChwYXJzZUludChwYXJ0c1sxXSkgKyBqKSArICcnO1xuICAgICAgICBjb25zdCBuc3NyYzIgPSAocGFyc2VJbnQocGFydHNbMl0pICsgaikgKyAnJztcbiAgICAgICAgc2ltR3JvdXBMaW5lcy5wdXNoKFxuICAgICAgICAgIGxpbmUucmVwbGFjZShwYXJ0c1sxXSwgbnNzcmMxKS5yZXBsYWNlKHBhcnRzWzJdLCBuc3NyYzIpKTtcbiAgICAgIH1cbiAgICB9XG4gICAgaSsrO1xuICB9XG5cbiAgY29uc3QgaW5zZXJ0UG9zID0gaTtcbiAgc3NyY3MuZm9yRWFjaChzc3JjID0+IHtcbiAgICBpZiAoIWdzc3Jjcy5oYXMoc3NyYykpIHtcbiAgICAgIGxldCBncm91cExpbmUgPSAnYT1zc3JjLWdyb3VwOlNJTSc7XG4gICAgICBncm91cExpbmUgPSBncm91cExpbmUgKyAnICcgKyBzc3JjO1xuICAgICAgZm9yIChsZXQgaiA9IDE7IGogPCBudW1TdHJlYW1zOyBqKyspIHtcbiAgICAgICAgZ3JvdXBMaW5lID0gZ3JvdXBMaW5lICsgJyAnICsgKHBhcnNlSW50KHNzcmMpICsgaik7XG4gICAgICB9XG4gICAgICBzaW1Hcm91cExpbmVzLnB1c2goZ3JvdXBMaW5lKTtcbiAgICB9XG4gIH0pO1xuXG4gIHNpbUxpbmVzLnNvcnQoKTtcbiAgLy8gSW5zZXJ0IHNpbXVsY2FzdCBzc3JjIGxpbmVzLlxuICBzZHBMaW5lcy5zcGxpY2UoaW5zZXJ0UG9zLCAwLCAuLi5zaW1Hcm91cExpbmVzKTtcbiAgc2RwTGluZXMuc3BsaWNlKGluc2VydFBvcywgMCwgLi4uc2ltTGluZXMpO1xuICBzZHBMaW5lcyA9IHNkcExpbmVzLmZpbHRlcihsaW5lID0+ICFyZW1vdmVzLmhhcyhsaW5lKSk7XG5cbiAgaWYgKGhlYWRMaW5lcykge1xuICAgIHNkcExpbmVzID0gaGVhZExpbmVzLmNvbmNhdChzZHBMaW5lcykuY29uY2F0KHRhaWxMaW5lcyk7XG4gIH1cbiAgc2RwID0gc2RwTGluZXMuam9pbignXFxyXFxuJyk7XG4gIHJldHVybiBzZHA7XG59XG5cbmV4cG9ydCBmdW5jdGlvbiBzZXRNYXhCaXRyYXRlKHNkcCwgZW5jb2RpbmdQYXJhbWV0ZXJzTGlzdCwgbWlkKSB7XG4gIGZvciAoY29uc3QgZW5jb2RpbmdQYXJhbWV0ZXJzIG9mIGVuY29kaW5nUGFyYW1ldGVyc0xpc3QpIHtcbiAgICBpZiAoZW5jb2RpbmdQYXJhbWV0ZXJzLm1heEJpdHJhdGUpIHtcbiAgICAgIHNkcCA9IHNldENvZGVjUGFyYW0oXG4gICAgICAgICAgc2RwLCBlbmNvZGluZ1BhcmFtZXRlcnMuY29kZWMubmFtZSwgJ3gtZ29vZ2xlLW1heC1iaXRyYXRlJyxcbiAgICAgICAgICAoZW5jb2RpbmdQYXJhbWV0ZXJzLm1heEJpdHJhdGUpLnRvU3RyaW5nKCksIG1pZCk7XG4gICAgfVxuICB9XG4gIHJldHVybiBzZHA7XG59XG4iLCIvLyBDb3B5cmlnaHQgKEMpIDwyMDE4PiBJbnRlbCBDb3Jwb3JhdGlvblxuLy9cbi8vIFNQRFgtTGljZW5zZS1JZGVudGlmaWVyOiBBcGFjaGUtMi4wXG5cbi8qIGdsb2JhbCBTZW5kU3RyZWFtLCBCaWRpcmVjdGlvbmFsU3RyZWFtICovXG5cbid1c2Ugc3RyaWN0JztcbmltcG9ydCAqIGFzIFV0aWxzIGZyb20gJy4vdXRpbHMuanMnO1xuaW1wb3J0IHtFdmVudERpc3BhdGNoZXIsIE93dEV2ZW50fSBmcm9tICcuL2V2ZW50LmpzJztcblxuLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbmZ1bmN0aW9uIGlzQWxsb3dlZFZhbHVlKG9iaiwgYWxsb3dlZFZhbHVlcykge1xuICByZXR1cm4gKGFsbG93ZWRWYWx1ZXMuc29tZSgoZWxlKSA9PiB7XG4gICAgcmV0dXJuIGVsZSA9PT0gb2JqO1xuICB9KSk7XG59XG4vKipcbiAqIEBjbGFzcyBTdHJlYW1Tb3VyY2VJbmZvXG4gKiBAbWVtYmVyT2YgT3d0LkJhc2VcbiAqIEBjbGFzc0Rlc2MgSW5mb3JtYXRpb24gb2YgYSBzdHJlYW0ncyBzb3VyY2UuXG4gKiBAY29uc3RydWN0b3JcbiAqIEBkZXNjcmlwdGlvbiBBdWRpbyBzb3VyY2UgaW5mbyBvciB2aWRlbyBzb3VyY2UgaW5mbyBjb3VsZCBiZSB1bmRlZmluZWQgaWZcbiAqIGEgc3RyZWFtIGRvZXMgbm90IGhhdmUgYXVkaW8vdmlkZW8gdHJhY2suXG4gKiBAcGFyYW0gez9zdHJpbmd9IGF1ZGlvU291cmNlSW5mbyBBdWRpbyBzb3VyY2UgaW5mby4gQWNjZXB0ZWQgdmFsdWVzIGFyZTpcbiAqIFwibWljXCIsIFwic2NyZWVuLWNhc3RcIiwgXCJmaWxlXCIsIFwibWl4ZWRcIiBvciB1bmRlZmluZWQuXG4gKiBAcGFyYW0gez9zdHJpbmd9IHZpZGVvU291cmNlSW5mbyBWaWRlbyBzb3VyY2UgaW5mby4gQWNjZXB0ZWQgdmFsdWVzIGFyZTpcbiAqIFwiY2FtZXJhXCIsIFwic2NyZWVuLWNhc3RcIiwgXCJmaWxlXCIsIFwibWl4ZWRcIiBvciB1bmRlZmluZWQuXG4gKiBAcGFyYW0ge2Jvb2xlYW59IGRhdGFTb3VyY2VJbmZvIEluZGljYXRlcyB3aGV0aGVyIGl0IGlzIGRhdGEuIEFjY2VwdGVkIHZhbHVlc1xuICogYXJlIGJvb2xlYW4uXG4gKi9cbmV4cG9ydCBjbGFzcyBTdHJlYW1Tb3VyY2VJbmZvIHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IoYXVkaW9Tb3VyY2VJbmZvLCB2aWRlb1NvdXJjZUluZm8sIGRhdGFTb3VyY2VJbmZvKSB7XG4gICAgaWYgKCFpc0FsbG93ZWRWYWx1ZShhdWRpb1NvdXJjZUluZm8sIFt1bmRlZmluZWQsICdtaWMnLCAnc2NyZWVuLWNhc3QnLFxuICAgICAgJ2ZpbGUnLCAnbWl4ZWQnXSkpIHtcbiAgICAgIHRocm93IG5ldyBUeXBlRXJyb3IoJ0luY29ycmVjdCB2YWx1ZSBmb3IgYXVkaW9Tb3VyY2VJbmZvJyk7XG4gICAgfVxuICAgIGlmICghaXNBbGxvd2VkVmFsdWUodmlkZW9Tb3VyY2VJbmZvLCBbdW5kZWZpbmVkLCAnY2FtZXJhJywgJ3NjcmVlbi1jYXN0JyxcbiAgICAgICdmaWxlJywgJ2VuY29kZWQtZmlsZScsICdyYXctZmlsZScsICdtaXhlZCddKSkge1xuICAgICAgdGhyb3cgbmV3IFR5cGVFcnJvcignSW5jb3JyZWN0IHZhbHVlIGZvciB2aWRlb1NvdXJjZUluZm8nKTtcbiAgICB9XG4gICAgdGhpcy5hdWRpbyA9IGF1ZGlvU291cmNlSW5mbztcbiAgICB0aGlzLnZpZGVvID0gdmlkZW9Tb3VyY2VJbmZvO1xuICAgIHRoaXMuZGF0YSA9IGRhdGFTb3VyY2VJbmZvO1xuICB9XG59XG4vKipcbiAqIEBjbGFzcyBTdHJlYW1cbiAqIEBtZW1iZXJPZiBPd3QuQmFzZVxuICogQGNsYXNzRGVzYyBCYXNlIGNsYXNzIG9mIHN0cmVhbXMuXG4gKiBAZXh0ZW5kcyBPd3QuQmFzZS5FdmVudERpc3BhdGNoZXJcbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNsYXNzIFN0cmVhbSBleHRlbmRzIEV2ZW50RGlzcGF0Y2hlciB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKHN0cmVhbSwgc291cmNlSW5mbywgYXR0cmlidXRlcykge1xuICAgIHN1cGVyKCk7XG4gICAgaWYgKChzdHJlYW0gJiYgIShzdHJlYW0gaW5zdGFuY2VvZiBNZWRpYVN0cmVhbSkgJiZcbiAgICAgICAgICEodHlwZW9mIFNlbmRTdHJlYW0gPT09ICdmdW5jdGlvbicgJiYgc3RyZWFtIGluc3RhbmNlb2YgU2VuZFN0cmVhbSkgJiZcbiAgICAgICAgICEodHlwZW9mIEJpZGlyZWN0aW9uYWxTdHJlYW0gPT09ICdmdW5jdGlvbicgJiZcbiAgICAgICAgICAgc3RyZWFtIGluc3RhbmNlb2YgQmlkaXJlY3Rpb25hbFN0cmVhbSkpIHx8XG4gICAgICAgICh0eXBlb2Ygc291cmNlSW5mbyAhPT0gJ29iamVjdCcpKSB7XG4gICAgICB0aHJvdyBuZXcgVHlwZUVycm9yKCdJbnZhbGlkIHN0cmVhbSBvciBzb3VyY2VJbmZvLicpO1xuICAgIH1cbiAgICBpZiAoc3RyZWFtICYmIChzdHJlYW0gaW5zdGFuY2VvZiBNZWRpYVN0cmVhbSkgJiZcbiAgICAgICAgKChzdHJlYW0uZ2V0QXVkaW9UcmFja3MoKS5sZW5ndGggPiAwICYmICFzb3VyY2VJbmZvLmF1ZGlvKSB8fFxuICAgICAgICAgc3RyZWFtLmdldFZpZGVvVHJhY2tzKCkubGVuZ3RoID4gMCAmJiAhc291cmNlSW5mby52aWRlbykpIHtcbiAgICAgIHRocm93IG5ldyBUeXBlRXJyb3IoJ01pc3NpbmcgYXVkaW8gc291cmNlIGluZm8gb3IgdmlkZW8gc291cmNlIGluZm8uJyk7XG4gICAgfVxuICAgIC8qKlxuICAgICAqIEBtZW1iZXIgez9NZWRpYVN0cmVhbX0gbWVkaWFTdHJlYW1cbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuU3RyZWFtXG4gICAgICogQHNlZSB7QGxpbmsgaHR0cHM6Ly93d3cudzMub3JnL1RSL21lZGlhY2FwdHVyZS1zdHJlYW1zLyNtZWRpYXN0cmVhbXxNZWRpYVN0cmVhbSBBUEkgb2YgTWVkaWEgQ2FwdHVyZSBhbmQgU3RyZWFtc30uXG4gICAgICogQGRlc2MgVGhpcyBwcm9wZXJ0eSBpcyBkZXByZWNhdGVkLCBwbGVhc2UgdXNlIHN0cmVhbSBpbnN0ZWFkLlxuICAgICAqL1xuICAgIGlmIChzdHJlYW0gaW5zdGFuY2VvZiBNZWRpYVN0cmVhbSkge1xuICAgICAgT2JqZWN0LmRlZmluZVByb3BlcnR5KHRoaXMsICdtZWRpYVN0cmVhbScsIHtcbiAgICAgICAgY29uZmlndXJhYmxlOiBmYWxzZSxcbiAgICAgICAgd3JpdGFibGU6IHRydWUsXG4gICAgICAgIHZhbHVlOiBzdHJlYW0sXG4gICAgICB9KTtcbiAgICB9XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7TWVkaWFTdHJlYW0gfCBTZW5kU3RyZWFtIHwgQmlkaXJlY3Rpb25hbFN0cmVhbSB8IHVuZGVmaW5lZH0gc3RyZWFtXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlN0cmVhbVxuICAgICAqIEBzZWUge0BsaW5rIGh0dHBzOi8vd3d3LnczLm9yZy9UUi9tZWRpYWNhcHR1cmUtc3RyZWFtcy8jbWVkaWFzdHJlYW18TWVkaWFTdHJlYW0gQVBJIG9mIE1lZGlhIENhcHR1cmUgYW5kIFN0cmVhbXN9XG4gICAgICogQHNlZSB7QGxpbmsgaHR0cHM6Ly93aWNnLmdpdGh1Yi5pby93ZWItdHJhbnNwb3J0LyBXZWJUcmFuc3BvcnR9LlxuICAgICAqL1xuICAgIE9iamVjdC5kZWZpbmVQcm9wZXJ0eSh0aGlzLCAnc3RyZWFtJywge1xuICAgICAgY29uZmlndXJhYmxlOiBmYWxzZSxcbiAgICAgIHdyaXRhYmxlOiB0cnVlLFxuICAgICAgdmFsdWU6IHN0cmVhbSxcbiAgICB9KTtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHtPd3QuQmFzZS5TdHJlYW1Tb3VyY2VJbmZvfSBzb3VyY2VcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuU3RyZWFtXG4gICAgICogQGRlc2MgU291cmNlIGluZm8gb2YgYSBzdHJlYW0uXG4gICAgICovXG4gICAgT2JqZWN0LmRlZmluZVByb3BlcnR5KHRoaXMsICdzb3VyY2UnLCB7XG4gICAgICBjb25maWd1cmFibGU6IGZhbHNlLFxuICAgICAgd3JpdGFibGU6IGZhbHNlLFxuICAgICAgdmFsdWU6IHNvdXJjZUluZm8sXG4gICAgfSk7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7b2JqZWN0fSBhdHRyaWJ1dGVzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlN0cmVhbVxuICAgICAqIEBkZXNjIEN1c3RvbSBhdHRyaWJ1dGVzIG9mIGEgc3RyZWFtLlxuICAgICAqL1xuICAgIE9iamVjdC5kZWZpbmVQcm9wZXJ0eSh0aGlzLCAnYXR0cmlidXRlcycsIHtcbiAgICAgIGNvbmZpZ3VyYWJsZTogdHJ1ZSxcbiAgICAgIHdyaXRhYmxlOiBmYWxzZSxcbiAgICAgIHZhbHVlOiBhdHRyaWJ1dGVzLFxuICAgIH0pO1xuICB9XG59XG4vKipcbiAqIEBjbGFzcyBMb2NhbFN0cmVhbVxuICogQGNsYXNzRGVzYyBTdHJlYW0gY2FwdHVyZWQgZnJvbSBjdXJyZW50IGVuZHBvaW50LlxuICogQG1lbWJlck9mIE93dC5CYXNlXG4gKiBAZXh0ZW5kcyBPd3QuQmFzZS5TdHJlYW1cbiAqIEBjb25zdHJ1Y3RvclxuICogQHBhcmFtIHtNZWRpYVN0cmVhbX0gc3RyZWFtIFVuZGVybHlpbmcgTWVkaWFTdHJlYW0uXG4gKiBAcGFyYW0ge093dC5CYXNlLlN0cmVhbVNvdXJjZUluZm99IHNvdXJjZUluZm8gSW5mb3JtYXRpb24gYWJvdXQgc3RyZWFtJ3NcbiAqIHNvdXJjZS5cbiAqIEBwYXJhbSB7b2JqZWN0fSBhdHRyaWJ1dGVzIEN1c3RvbSBhdHRyaWJ1dGVzIG9mIHRoZSBzdHJlYW0uXG4gKi9cbmV4cG9ydCBjbGFzcyBMb2NhbFN0cmVhbSBleHRlbmRzIFN0cmVhbSB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKHN0cmVhbSwgc291cmNlSW5mbywgYXR0cmlidXRlcykge1xuICAgIGlmICghc3RyZWFtKSB7XG4gICAgICB0aHJvdyBuZXcgVHlwZUVycm9yKCdTdHJlYW0gY2Fubm90IGJlIG51bGwuJyk7XG4gICAgfVxuICAgIHN1cGVyKHN0cmVhbSwgc291cmNlSW5mbywgYXR0cmlidXRlcyk7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7c3RyaW5nfSBpZFxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5Mb2NhbFN0cmVhbVxuICAgICAqL1xuICAgIE9iamVjdC5kZWZpbmVQcm9wZXJ0eSh0aGlzLCAnaWQnLCB7XG4gICAgICBjb25maWd1cmFibGU6IGZhbHNlLFxuICAgICAgd3JpdGFibGU6IGZhbHNlLFxuICAgICAgdmFsdWU6IFV0aWxzLmNyZWF0ZVV1aWQoKSxcbiAgICB9KTtcbiAgfVxufVxuLyoqXG4gKiBAY2xhc3MgUmVtb3RlU3RyZWFtXG4gKiBAY2xhc3NEZXNjIFN0cmVhbSBzZW50IGZyb20gYSByZW1vdGUgZW5kcG9pbnQuIEluIGNvbmZlcmVuY2UgbW9kZSxcbiAqIFJlbW90ZVN0cmVhbSdzIHN0cmVhbSBpcyBhbHdheXMgdW5kZWZpbmVkLiBQbGVhc2UgZ2V0IE1lZGlhU3RyZWFtIG9yXG4gKiBSZWFkYWJsZVN0cmVhbSBmcm9tIHN1YnNjcmlwdGlvbidzIHN0cmVhbSBwcm9wZXJ0eS5cbiAqIEV2ZW50czpcbiAqXG4gKiB8IEV2ZW50IE5hbWUgICAgICB8IEFyZ3VtZW50IFR5cGUgICAgfCBGaXJlZCB3aGVuICAgICAgICAgfFxuICogfCAtLS0tLS0tLS0tLS0tLS0tfCAtLS0tLS0tLS0tLS0tLS0tIHwgLS0tLS0tLS0tLS0tLS0tLS0tIHxcbiAqIHwgZW5kZWQgICAgICAgICAgIHwgRXZlbnQgICAgICAgICAgICB8IFN0cmVhbSBpcyBubyBsb25nZXIgYXZhaWxhYmxlIG9uIHNlcnZlciBzaWRlLiAgIHxcbiAqIHwgdXBkYXRlZCAgICAgICAgIHwgRXZlbnQgICAgICAgICAgICB8IFN0cmVhbSBpcyB1cGRhdGVkLiB8XG4gKlxuICogQG1lbWJlck9mIE93dC5CYXNlXG4gKiBAZXh0ZW5kcyBPd3QuQmFzZS5TdHJlYW1cbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNsYXNzIFJlbW90ZVN0cmVhbSBleHRlbmRzIFN0cmVhbSB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKGlkLCBvcmlnaW4sIHN0cmVhbSwgc291cmNlSW5mbywgYXR0cmlidXRlcykge1xuICAgIHN1cGVyKHN0cmVhbSwgc291cmNlSW5mbywgYXR0cmlidXRlcyk7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7c3RyaW5nfSBpZFxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5SZW1vdGVTdHJlYW1cbiAgICAgKi9cbiAgICBPYmplY3QuZGVmaW5lUHJvcGVydHkodGhpcywgJ2lkJywge1xuICAgICAgY29uZmlndXJhYmxlOiBmYWxzZSxcbiAgICAgIHdyaXRhYmxlOiBmYWxzZSxcbiAgICAgIHZhbHVlOiBpZCA/IGlkIDogVXRpbHMuY3JlYXRlVXVpZCgpLFxuICAgIH0pO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge3N0cmluZ30gb3JpZ2luXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlJlbW90ZVN0cmVhbVxuICAgICAqIEBkZXNjIElEIG9mIHRoZSByZW1vdGUgZW5kcG9pbnQgd2hvIHB1Ymxpc2hlZCB0aGlzIHN0cmVhbS5cbiAgICAgKi9cbiAgICBPYmplY3QuZGVmaW5lUHJvcGVydHkodGhpcywgJ29yaWdpbicsIHtcbiAgICAgIGNvbmZpZ3VyYWJsZTogZmFsc2UsXG4gICAgICB3cml0YWJsZTogZmFsc2UsXG4gICAgICB2YWx1ZTogb3JpZ2luLFxuICAgIH0pO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge093dC5CYXNlLlB1YmxpY2F0aW9uU2V0dGluZ3N9IHNldHRpbmdzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlJlbW90ZVN0cmVhbVxuICAgICAqIEBkZXNjIE9yaWdpbmFsIHNldHRpbmdzIGZvciBwdWJsaXNoaW5nIHRoaXMgc3RyZWFtLiBUaGlzIHByb3BlcnR5IGlzIG9ubHlcbiAgICAgKiB2YWxpZCBpbiBjb25mZXJlbmNlIG1vZGUuXG4gICAgICovXG4gICAgdGhpcy5zZXR0aW5ncyA9IHVuZGVmaW5lZDtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHtPd3QuQ29uZmVyZW5jZS5TdWJzY3JpcHRpb25DYXBhYmlsaXRpZXN9IGV4dHJhQ2FwYWJpbGl0aWVzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlJlbW90ZVN0cmVhbVxuICAgICAqIEBkZXNjIEV4dHJhIGNhcGFiaWxpdGllcyByZW1vdGUgZW5kcG9pbnQgcHJvdmlkZXMgZm9yIHN1YnNjcmlwdGlvbi4gRXh0cmFcbiAgICAgKiBjYXBhYmlsaXRpZXMgZG9uJ3QgaW5jbHVkZSBvcmlnaW5hbCBzZXR0aW5ncy4gVGhpcyBwcm9wZXJ0eSBpcyBvbmx5IHZhbGlkXG4gICAgICogaW4gY29uZmVyZW5jZSBtb2RlLlxuICAgICAqL1xuICAgIHRoaXMuZXh0cmFDYXBhYmlsaXRpZXMgPSB1bmRlZmluZWQ7XG4gIH1cbn1cblxuLyoqXG4gKiBAY2xhc3MgU3RyZWFtRXZlbnRcbiAqIEBjbGFzc0Rlc2MgRXZlbnQgZm9yIFN0cmVhbS5cbiAqIEBleHRlbmRzIE93dC5CYXNlLk93dEV2ZW50XG4gKiBAbWVtYmVyb2YgT3d0LkJhc2VcbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNsYXNzIFN0cmVhbUV2ZW50IGV4dGVuZHMgT3d0RXZlbnQge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3Rvcih0eXBlLCBpbml0KSB7XG4gICAgc3VwZXIodHlwZSk7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7T3d0LkJhc2UuU3RyZWFtfSBzdHJlYW1cbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuU3RyZWFtRXZlbnRcbiAgICAgKi9cbiAgICB0aGlzLnN0cmVhbSA9IGluaXQuc3RyZWFtO1xuICB9XG59XG4iLCIvLyBDb3B5cmlnaHQgKEMpIDwyMDIwPiBJbnRlbCBDb3Jwb3JhdGlvblxuLy9cbi8vIFNQRFgtTGljZW5zZS1JZGVudGlmaWVyOiBBcGFjaGUtMi4wXG5cbid1c2Ugc3RyaWN0JztcblxuLyoqXG4gKiBAY2xhc3MgVHJhbnNwb3J0VHlwZVxuICogQG1lbWJlck9mIE93dC5CYXNlXG4gKiBAY2xhc3NEZXNjIFRyYW5zcG9ydCB0eXBlIGVudW1lcmF0aW9uLlxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY29uc3QgVHJhbnNwb3J0VHlwZSA9IHtcbiAgUVVJQzogJ3F1aWMnLFxuICBXRUJSVEM6ICd3ZWJydGMnLFxufTtcblxuLyoqXG4gKiBAY2xhc3MgVHJhbnNwb3J0Q29uc3RyYWludHNcbiAqIEBtZW1iZXJPZiBPd3QuQmFzZVxuICogQGNsYXNzRGVzYyBSZXByZXNlbnRzIHRoZSB0cmFuc3BvcnQgY29uc3RyYWludHMgZm9yIHB1YmxpY2F0aW9uIGFuZFxuICogc3Vic2NyaXB0aW9uLlxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY2xhc3MgVHJhbnNwb3J0Q29uc3RyYWludHMge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3Rvcih0eXBlLCBpZCkge1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge0FycmF5LjxPd3QuQmFzZS5UcmFuc3BvcnRUeXBlPn0gdHlwZVxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5UcmFuc3BvcnRDb25zdHJhaW50c1xuICAgICAqIEBkZXNjIFRyYW5zcG9ydCB0eXBlIGZvciBwdWJsaWNhdGlvbiBhbmQgc3Vic2NyaXB0aW9uLlxuICAgICAqL1xuICAgIHRoaXMudHlwZSA9IHR5cGU7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P3N0cmluZ30gaWRcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkJhc2UuVHJhbnNwb3J0Q29uc3RyYWludHNcbiAgICAgKiBAZGVzYyBUcmFuc3BvcnQgSUQuIFVuZGVmaW5lZCB0cmFuc3BvcnQgSUQgcmVzdWx0cyBzZXJ2ZXIgdG8gYXNzaWduIGEgbmV3XG4gICAgICogb25lLiBJdCBzaG91bGQgYWx3YXlzIGJlIHVuZGVmaW5lZCBpZiB0cmFuc3BvcnQgdHlwZSBpcyB3ZWJydGMgc2luY2UgdGhlXG4gICAgICogd2VicnRjIGFnZW50IG9mIE9XVCBzZXJ2ZXIgZG9lc24ndCBzdXBwb3J0IG11bHRpcGxlIHRyYW5zY2VpdmVycyBvbiBhXG4gICAgICogc2luZ2xlIFBlZXJDb25uZWN0aW9uLlxuICAgICAqL1xuICAgIHRoaXMuaWQgPSBpZDtcbiAgfVxufVxuXG4vKipcbiAqIEBjbGFzcyBUcmFuc3BvcnRTZXR0aW5nc1xuICogQG1lbWJlck9mIE93dC5CYXNlXG4gKiBAY2xhc3NEZXNjIFJlcHJlc2VudHMgdGhlIHRyYW5zcG9ydCBzZXR0aW5ncyBmb3IgcHVibGljYXRpb24gYW5kXG4gKiBzdWJzY3JpcHRpb24uXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKi9cbmV4cG9ydCBjbGFzcyBUcmFuc3BvcnRTZXR0aW5ncyB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKHR5cGUsIGlkKSB7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7T3d0LkJhc2UuVHJhbnNwb3J0VHlwZX0gdHlwZVxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5UcmFuc3BvcnRTZXR0aW5nc1xuICAgICAqIEBkZXNjIFRyYW5zcG9ydCB0eXBlIGZvciBwdWJsaWNhdGlvbiBhbmQgc3Vic2NyaXB0aW9uLlxuICAgICAqL1xuICAgIHRoaXMudHlwZSA9IHR5cGU7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7c3RyaW5nfSBpZFxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5UcmFuc3BvcnRTZXR0aW5nc1xuICAgICAqIEBkZXNjIFRyYW5zcG9ydCBJRC5cbiAgICAgKi9cbiAgICB0aGlzLmlkID0gaWQ7XG5cbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/QXJyYXkuPFJUQ1J0cFRyYW5zY2VpdmVyPn0gcnRwVHJhbnNjZWl2ZXJzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5CYXNlLlRyYW5zcG9ydFNldHRpbmdzXG4gICAgICogQGRlc2MgQSBsaXN0IG9mIFJUQ1J0cFRyYW5zY2VpdmVyIGFzc29jaWF0ZWQgd2l0aCB0aGUgcHVibGljYXRpb24gb3JcbiAgICAgKiBzdWJzY3JpcHRpb24uIEl0J3Mgb25seSBhdmFpbGFibGUgaW4gY29uZmVyZW5jZSBtb2RlIHdoZW4gVHJhbnNwb3J0VHlwZVxuICAgICAqIGlzIHdlYnJ0Yy5cbiAgICAgKiBAc2VlIHtAbGluayBodHRwczovL3czYy5naXRodWIuaW8vd2VicnRjLXBjLyNydGNydHB0cmFuc2NlaXZlci1pbnRlcmZhY2V8UlRDUnRwVHJhbnNjZWl2ZXIgSW50ZXJmYWNlIG9mIFdlYlJUQyAxLjB9LlxuICAgICAqL1xuICAgIHRoaXMucnRwVHJhbnNjZWl2ZXJzID0gdW5kZWZpbmVkO1xuICB9XG59XG4iLCIvLyBDb3B5cmlnaHQgKEMpIDwyMDE4PiBJbnRlbCBDb3Jwb3JhdGlvblxuLy9cbi8vIFNQRFgtTGljZW5zZS1JZGVudGlmaWVyOiBBcGFjaGUtMi4wXG5cbi8qIGdsb2JhbCBuYXZpZ2F0b3IsIHdpbmRvdyAqL1xuXG4ndXNlIHN0cmljdCc7XG5jb25zdCBzZGtWZXJzaW9uID0gJzUuMCc7XG5cbi8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG5leHBvcnQgZnVuY3Rpb24gaXNGaXJlZm94KCkge1xuICByZXR1cm4gd2luZG93Lm5hdmlnYXRvci51c2VyQWdlbnQubWF0Y2goJ0ZpcmVmb3gnKSAhPT0gbnVsbDtcbn1cbi8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG5leHBvcnQgZnVuY3Rpb24gaXNDaHJvbWUoKSB7XG4gIHJldHVybiB3aW5kb3cubmF2aWdhdG9yLnVzZXJBZ2VudC5tYXRjaCgnQ2hyb21lJykgIT09IG51bGw7XG59XG4vLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuZXhwb3J0IGZ1bmN0aW9uIGlzU2FmYXJpKCkge1xuICByZXR1cm4gL14oKD8hY2hyb21lfGFuZHJvaWQpLikqc2FmYXJpL2kudGVzdCh3aW5kb3cubmF2aWdhdG9yLnVzZXJBZ2VudCk7XG59XG4vLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuZXhwb3J0IGZ1bmN0aW9uIGlzRWRnZSgpIHtcbiAgcmV0dXJuIHdpbmRvdy5uYXZpZ2F0b3IudXNlckFnZW50Lm1hdGNoKC9FZGdlXFwvKFxcZCspLihcXGQrKSQvKSAhPT0gbnVsbDtcbn1cbi8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG5leHBvcnQgZnVuY3Rpb24gY3JlYXRlVXVpZCgpIHtcbiAgcmV0dXJuICd4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eCcucmVwbGFjZSgvW3h5XS9nLCBmdW5jdGlvbihjKSB7XG4gICAgY29uc3QgciA9IE1hdGgucmFuZG9tKCkgKiAxNiB8IDA7XG4gICAgY29uc3QgdiA9IGMgPT09ICd4JyA/IHIgOiAociAmIDB4MyB8IDB4OCk7XG4gICAgcmV0dXJuIHYudG9TdHJpbmcoMTYpO1xuICB9KTtcbn1cblxuLy8gUmV0dXJucyBzeXN0ZW0gaW5mb3JtYXRpb24uXG4vLyBGb3JtYXQ6IHtzZGs6e3ZlcnNpb246KiosIHR5cGU6Kip9LCBydW50aW1lOnt2ZXJzaW9uOioqLCBuYW1lOioqfSwgb3M6e3ZlcnNpb246KiosIG5hbWU6Kip9fTtcbi8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG5leHBvcnQgZnVuY3Rpb24gc3lzSW5mbygpIHtcbiAgY29uc3QgaW5mbyA9IE9iamVjdC5jcmVhdGUoe30pO1xuICBpbmZvLnNkayA9IHtcbiAgICB2ZXJzaW9uOiBzZGtWZXJzaW9uLFxuICAgIHR5cGU6ICdKYXZhU2NyaXB0JyxcbiAgfTtcbiAgLy8gUnVudGltZSBpbmZvLlxuICBjb25zdCB1c2VyQWdlbnQgPSBuYXZpZ2F0b3IudXNlckFnZW50O1xuICBjb25zdCBmaXJlZm94UmVnZXggPSAvRmlyZWZveFxcLyhbMC05Ll0rKS87XG4gIGNvbnN0IGNocm9tZVJlZ2V4ID0gL0Nocm9tZVxcLyhbMC05Ll0rKS87XG4gIGNvbnN0IGVkZ2VSZWdleCA9IC9FZGdlXFwvKFswLTkuXSspLztcbiAgY29uc3Qgc2FmYXJpVmVyc2lvblJlZ2V4ID0gL1ZlcnNpb25cXC8oWzAtOS5dKykgU2FmYXJpLztcbiAgbGV0IHJlc3VsdCA9IGNocm9tZVJlZ2V4LmV4ZWModXNlckFnZW50KTtcbiAgaWYgKHJlc3VsdCkge1xuICAgIGluZm8ucnVudGltZSA9IHtcbiAgICAgIG5hbWU6ICdDaHJvbWUnLFxuICAgICAgdmVyc2lvbjogcmVzdWx0WzFdLFxuICAgIH07XG4gIH0gZWxzZSBpZiAocmVzdWx0ID0gZmlyZWZveFJlZ2V4LmV4ZWModXNlckFnZW50KSkge1xuICAgIGluZm8ucnVudGltZSA9IHtcbiAgICAgIG5hbWU6ICdGaXJlZm94JyxcbiAgICAgIHZlcnNpb246IHJlc3VsdFsxXSxcbiAgICB9O1xuICB9IGVsc2UgaWYgKHJlc3VsdCA9IGVkZ2VSZWdleC5leGVjKHVzZXJBZ2VudCkpIHtcbiAgICBpbmZvLnJ1bnRpbWUgPSB7XG4gICAgICBuYW1lOiAnRWRnZScsXG4gICAgICB2ZXJzaW9uOiByZXN1bHRbMV0sXG4gICAgfTtcbiAgfSBlbHNlIGlmIChpc1NhZmFyaSgpKSB7XG4gICAgcmVzdWx0ID0gc2FmYXJpVmVyc2lvblJlZ2V4LmV4ZWModXNlckFnZW50KTtcbiAgICBpbmZvLnJ1bnRpbWUgPSB7XG4gICAgICBuYW1lOiAnU2FmYXJpJyxcbiAgICB9O1xuICAgIGluZm8ucnVudGltZS52ZXJzaW9uID0gcmVzdWx0ID8gcmVzdWx0WzFdIDogJ1Vua25vd24nO1xuICB9IGVsc2Uge1xuICAgIGluZm8ucnVudGltZSA9IHtcbiAgICAgIG5hbWU6ICdVbmtub3duJyxcbiAgICAgIHZlcnNpb246ICdVbmtub3duJyxcbiAgICB9O1xuICB9XG4gIC8vIE9TIGluZm8uXG4gIGNvbnN0IHdpbmRvd3NSZWdleCA9IC9XaW5kb3dzIE5UIChbMC05Ll0rKS87XG4gIGNvbnN0IG1hY1JlZ2V4ID0gL0ludGVsIE1hYyBPUyBYIChbMC05Xy5dKykvO1xuICBjb25zdCBpUGhvbmVSZWdleCA9IC9pUGhvbmUgT1MgKFswLTlfLl0rKS87XG4gIGNvbnN0IGxpbnV4UmVnZXggPSAvWDExOyBMaW51eC87XG4gIGNvbnN0IGFuZHJvaWRSZWdleCA9IC9BbmRyb2lkKCAoWzAtOS5dKykpPy87XG4gIGNvbnN0IGNocm9taXVtT3NSZWdleCA9IC9Dck9TLztcbiAgaWYgKHJlc3VsdCA9IHdpbmRvd3NSZWdleC5leGVjKHVzZXJBZ2VudCkpIHtcbiAgICBpbmZvLm9zID0ge1xuICAgICAgbmFtZTogJ1dpbmRvd3MgTlQnLFxuICAgICAgdmVyc2lvbjogcmVzdWx0WzFdLFxuICAgIH07XG4gIH0gZWxzZSBpZiAocmVzdWx0ID0gbWFjUmVnZXguZXhlYyh1c2VyQWdlbnQpKSB7XG4gICAgaW5mby5vcyA9IHtcbiAgICAgIG5hbWU6ICdNYWMgT1MgWCcsXG4gICAgICB2ZXJzaW9uOiByZXN1bHRbMV0ucmVwbGFjZSgvXy9nLCAnLicpLFxuICAgIH07XG4gIH0gZWxzZSBpZiAocmVzdWx0ID0gaVBob25lUmVnZXguZXhlYyh1c2VyQWdlbnQpKSB7XG4gICAgaW5mby5vcyA9IHtcbiAgICAgIG5hbWU6ICdpUGhvbmUgT1MnLFxuICAgICAgdmVyc2lvbjogcmVzdWx0WzFdLnJlcGxhY2UoL18vZywgJy4nKSxcbiAgICB9O1xuICB9IGVsc2UgaWYgKHJlc3VsdCA9IGxpbnV4UmVnZXguZXhlYyh1c2VyQWdlbnQpKSB7XG4gICAgaW5mby5vcyA9IHtcbiAgICAgIG5hbWU6ICdMaW51eCcsXG4gICAgICB2ZXJzaW9uOiAnVW5rbm93bicsXG4gICAgfTtcbiAgfSBlbHNlIGlmIChyZXN1bHQgPSBhbmRyb2lkUmVnZXguZXhlYyh1c2VyQWdlbnQpKSB7XG4gICAgaW5mby5vcyA9IHtcbiAgICAgIG5hbWU6ICdBbmRyb2lkJyxcbiAgICAgIHZlcnNpb246IHJlc3VsdFsxXSB8fCAnVW5rbm93bicsXG4gICAgfTtcbiAgfSBlbHNlIGlmIChyZXN1bHQgPSBjaHJvbWl1bU9zUmVnZXguZXhlYyh1c2VyQWdlbnQpKSB7XG4gICAgaW5mby5vcyA9IHtcbiAgICAgIG5hbWU6ICdDaHJvbWUgT1MnLFxuICAgICAgdmVyc2lvbjogJ1Vua25vd24nLFxuICAgIH07XG4gIH0gZWxzZSB7XG4gICAgaW5mby5vcyA9IHtcbiAgICAgIG5hbWU6ICdVbmtub3duJyxcbiAgICAgIHZlcnNpb246ICdVbmtub3duJyxcbiAgICB9O1xuICB9XG4gIGluZm8uY2FwYWJpbGl0aWVzID0ge1xuICAgIGNvbnRpbnVhbEljZUdhdGhlcmluZzogZmFsc2UsXG4gICAgc3RyZWFtUmVtb3ZhYmxlOiBpbmZvLnJ1bnRpbWUubmFtZSAhPT0gJ0ZpcmVmb3gnLFxuICAgIGlnbm9yZURhdGFDaGFubmVsQWNrczogdHJ1ZSxcbiAgfTtcbiAgcmV0dXJuIGluZm87XG59XG4iLCIvLyBDb3B5cmlnaHQgKEMpIDwyMDE4PiBJbnRlbCBDb3Jwb3JhdGlvblxuLy9cbi8vIFNQRFgtTGljZW5zZS1JZGVudGlmaWVyOiBBcGFjaGUtMi4wXG5cbi8qIGVzbGludC1kaXNhYmxlIHJlcXVpcmUtanNkb2MgKi9cbi8qIGdsb2JhbCBNYXAsIFByb21pc2UsIHNldFRpbWVvdXQgKi9cblxuJ3VzZSBzdHJpY3QnO1xuXG5pbXBvcnQgTG9nZ2VyIGZyb20gJy4uL2Jhc2UvbG9nZ2VyLmpzJztcbmltcG9ydCB7XG4gIEV2ZW50RGlzcGF0Y2hlcixcbiAgTWVzc2FnZUV2ZW50LFxuICBPd3RFdmVudCxcbiAgRXJyb3JFdmVudCxcbiAgTXV0ZUV2ZW50LFxufSBmcm9tICcuLi9iYXNlL2V2ZW50LmpzJztcbmltcG9ydCB7VHJhY2tLaW5kfSBmcm9tICcuLi9iYXNlL21lZGlhZm9ybWF0LmpzJztcbmltcG9ydCB7UHVibGljYXRpb259IGZyb20gJy4uL2Jhc2UvcHVibGljYXRpb24uanMnO1xuaW1wb3J0IHtTdWJzY3JpcHRpb259IGZyb20gJy4vc3Vic2NyaXB0aW9uLmpzJztcbmltcG9ydCB7Q29uZmVyZW5jZUVycm9yfSBmcm9tICcuL2Vycm9yLmpzJztcbmltcG9ydCAqIGFzIFV0aWxzIGZyb20gJy4uL2Jhc2UvdXRpbHMuanMnO1xuaW1wb3J0ICogYXMgU2RwVXRpbHMgZnJvbSAnLi4vYmFzZS9zZHB1dGlscy5qcyc7XG5pbXBvcnQge1RyYW5zcG9ydFNldHRpbmdzLCBUcmFuc3BvcnRUeXBlfSBmcm9tICcuLi9iYXNlL3RyYW5zcG9ydC5qcyc7XG5cbi8qKlxuICogQGNsYXNzIENvbmZlcmVuY2VQZWVyQ29ubmVjdGlvbkNoYW5uZWxcbiAqIEBjbGFzc0Rlc2MgQSBjaGFubmVsIGZvciBhIGNvbm5lY3Rpb24gYmV0d2VlbiBjbGllbnQgYW5kIGNvbmZlcmVuY2Ugc2VydmVyLlxuICogQ3VycmVudGx5LCBvbmx5IG9uZSBzdHJlYW0gY291bGQgYmUgdHJhbm1pdHRlZCBpbiBhIGNoYW5uZWwuXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKiBAcHJpdmF0ZVxuICovXG5leHBvcnQgY2xhc3MgQ29uZmVyZW5jZVBlZXJDb25uZWN0aW9uQ2hhbm5lbCBleHRlbmRzIEV2ZW50RGlzcGF0Y2hlciB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKGNvbmZpZywgc2lnbmFsaW5nKSB7XG4gICAgc3VwZXIoKTtcbiAgICB0aGlzLnBjID0gbnVsbDtcbiAgICB0aGlzLl9jb25maWcgPSBjb25maWc7XG4gICAgdGhpcy5fdmlkZW9Db2RlY3MgPSB1bmRlZmluZWQ7XG4gICAgdGhpcy5fb3B0aW9ucyA9IG51bGw7XG4gICAgdGhpcy5fdmlkZW9Db2RlY3MgPSB1bmRlZmluZWQ7XG4gICAgdGhpcy5fc2lnbmFsaW5nID0gc2lnbmFsaW5nO1xuICAgIHRoaXMuX2ludGVybmFsSWQgPSBudWxsOyAvLyBJdCdzIHB1YmxpY2F0aW9uIElEIG9yIHN1YnNjcmlwdGlvbiBJRC5cbiAgICB0aGlzLl9wZW5kaW5nQ2FuZGlkYXRlcyA9IFtdO1xuICAgIHRoaXMuX3N1YnNjcmliZVByb21pc2VzID0gbmV3IE1hcCgpOyAvLyBpbnRlcm5hbElkID0+IHsgcmVzb2x2ZSwgcmVqZWN0IH1cbiAgICB0aGlzLl9wdWJsaXNoUHJvbWlzZXMgPSBuZXcgTWFwKCk7IC8vIGludGVybmFsSWQgPT4geyByZXNvbHZlLCByZWplY3QgfVxuICAgIHRoaXMuX3B1YmxpY2F0aW9ucyA9IG5ldyBNYXAoKTsgLy8gUHVibGljYXRpb25JZCA9PiBQdWJsaWNhdGlvblxuICAgIHRoaXMuX3N1YnNjcmlwdGlvbnMgPSBuZXcgTWFwKCk7IC8vIFN1YnNjcmlwdGlvbklkID0+IFN1YnNjcmlwdGlvblxuICAgIHRoaXMuX3B1Ymxpc2hUcmFuc2NlaXZlcnMgPSBuZXcgTWFwKCk7IC8vIGludGVybmFsSWQgPT4geyBpZCwgdHJhbnNjZWl2ZXJzOiBbVHJhbnNjZWl2ZXJdIH1cbiAgICB0aGlzLl9zdWJzY3JpYmVUcmFuc2NlaXZlcnMgPSBuZXcgTWFwKCk7IC8vIGludGVybmFsSWQgPT4geyBpZCwgdHJhbnNjZWl2ZXJzOiBbVHJhbnNjZWl2ZXJdIH1cbiAgICB0aGlzLl9yZXZlcnNlSWRNYXAgPSBuZXcgTWFwKCk7IC8vIFB1YmxpY2F0aW9uSWQgfHwgU3Vic2NyaXB0aW9uSWQgPT4gaW50ZXJuYWxJZFxuICAgIC8vIFRpbWVyIGZvciBQZWVyQ29ubmVjdGlvbiBkaXNjb25uZWN0ZWQuIFdpbGwgc3RvcCBjb25uZWN0aW9uIGFmdGVyIHRpbWVyLlxuICAgIHRoaXMuX2Rpc2Nvbm5lY3RUaW1lciA9IG51bGw7XG4gICAgdGhpcy5fZW5kZWQgPSBmYWxzZTtcbiAgICAvLyBDaGFubmVsIElEIGFzc2lnbmVkIGJ5IGNvbmZlcmVuY2VcbiAgICB0aGlzLl9pZCA9IHVuZGVmaW5lZDtcbiAgICAvLyBVc2VkIHRvIGNyZWF0ZSBpbnRlcm5hbCBJRCBmb3IgcHVibGljYXRpb24vc3Vic2NyaXB0aW9uXG4gICAgdGhpcy5faW50ZXJuYWxDb3VudCA9IDA7XG4gICAgdGhpcy5fc2RwUHJvbWlzZSA9IFByb21pc2UucmVzb2x2ZSgpO1xuICAgIHRoaXMuX3NkcFJlc29sdmVyTWFwID0gbmV3IE1hcCgpOyAvLyBpbnRlcm5hbElkID0+IHtmaW5pc2gsIHJlc29sdmUsIHJlamVjdH1cbiAgICB0aGlzLl9zZHBSZXNvbHZlcnMgPSBbXTsgLy8gW3tmaW5pc2gsIHJlc29sdmUsIHJlamVjdH1dXG4gICAgdGhpcy5fc2RwUmVzb2x2ZU51bSA9IDA7XG4gICAgdGhpcy5fcmVtb3RlTWVkaWFTdHJlYW1zID0gbmV3IE1hcCgpOyAvLyBLZXkgaXMgc3Vic2NyaXB0aW9uIElELCB2YWx1ZSBpcyBNZWRpYVN0cmVhbS5cbiAgfVxuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gb25NZXNzYWdlXG4gICAqIEBkZXNjIFJlY2VpdmVkIGEgbWVzc2FnZSBmcm9tIGNvbmZlcmVuY2UgcG9ydGFsLiBEZWZpbmVkIGluIGNsaWVudC1zZXJ2ZXJcbiAgICogcHJvdG9jb2wuXG4gICAqIEBwYXJhbSB7c3RyaW5nfSBub3RpZmljYXRpb24gTm90aWZpY2F0aW9uIHR5cGUuXG4gICAqIEBwYXJhbSB7b2JqZWN0fSBtZXNzYWdlIE1lc3NhZ2UgcmVjZWl2ZWQuXG4gICAqIEBwcml2YXRlXG4gICAqL1xuICBvbk1lc3NhZ2Uobm90aWZpY2F0aW9uLCBtZXNzYWdlKSB7XG4gICAgc3dpdGNoIChub3RpZmljYXRpb24pIHtcbiAgICAgIGNhc2UgJ3Byb2dyZXNzJzpcbiAgICAgICAgaWYgKG1lc3NhZ2Uuc3RhdHVzID09PSAnc29hYycpIHtcbiAgICAgICAgICB0aGlzLl9zZHBIYW5kbGVyKG1lc3NhZ2UuZGF0YSk7XG4gICAgICAgIH0gZWxzZSBpZiAobWVzc2FnZS5zdGF0dXMgPT09ICdyZWFkeScpIHtcbiAgICAgICAgICB0aGlzLl9yZWFkeUhhbmRsZXIobWVzc2FnZS5zZXNzaW9uSWQpO1xuICAgICAgICB9IGVsc2UgaWYgKG1lc3NhZ2Uuc3RhdHVzID09PSAnZXJyb3InKSB7XG4gICAgICAgICAgdGhpcy5fZXJyb3JIYW5kbGVyKG1lc3NhZ2Uuc2Vzc2lvbklkLCBtZXNzYWdlLmRhdGEpO1xuICAgICAgICB9XG4gICAgICAgIGJyZWFrO1xuICAgICAgY2FzZSAnc3RyZWFtJzpcbiAgICAgICAgdGhpcy5fb25TdHJlYW1FdmVudChtZXNzYWdlKTtcbiAgICAgICAgYnJlYWs7XG4gICAgICBkZWZhdWx0OlxuICAgICAgICBMb2dnZXIud2FybmluZygnVW5rbm93biBub3RpZmljYXRpb24gZnJvbSBNQ1UuJyk7XG4gICAgfVxuICB9XG5cbiAgYXN5bmMgcHVibGlzaFdpdGhUcmFuc2NlaXZlcnMoc3RyZWFtLCB0cmFuc2NlaXZlcnMpIHtcbiAgICBmb3IgKGNvbnN0IHQgb2YgdHJhbnNjZWl2ZXJzKSB7XG4gICAgICBpZiAodC5kaXJlY3Rpb24gIT09ICdzZW5kb25seScpIHtcbiAgICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KFxuICAgICAgICAgICAgJ1JUQ1J0cFRyYW5zY2VpdmVyXFwncyBkaXJlY3Rpb24gbXVzdCBiZSBzZW5kb25seS4nKTtcbiAgICAgIH1cbiAgICAgIGlmICghc3RyZWFtLnN0cmVhbS5nZXRUcmFja3MoKS5pbmNsdWRlcyh0LnNlbmRlci50cmFjaykpIHtcbiAgICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KFxuICAgICAgICAgICAgJ1RoZSB0cmFjayBhc3NvY2lhdGVkIHdpdGggUlRDUnRwU2VuZGVyIGlzIG5vdCBpbmNsdWRlZCBpbiAnICtcbiAgICAgICAgICAgICdzdHJlYW0uJyk7XG4gICAgICB9XG4gICAgICBpZiAodHJhbnNjZWl2ZXJzLmxlbmd0aCA+IDIpIHtcbiAgICAgICAgLy8gTm90IHN1cHBvcnRlZCBieSBzZXJ2ZXIuXG4gICAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChcbiAgICAgICAgICAgICdBdCBtb3N0IG9uZSB0cmFuc2NlaXZlciBmb3IgYXVkaW8gYW5kIG9uZSB0cmFuc2NlaXZlciBmb3IgdmlkZW8gJyArXG4gICAgICAgICAgICAnYXJlIGFjY2VwdGVkLicpO1xuICAgICAgfVxuICAgICAgY29uc3QgdHJhbnNjZWl2ZXJEZXNjcmlwdGlvbiA9IHRyYW5zY2VpdmVycy5tYXAoKHQpID0+IHtcbiAgICAgICAgY29uc3Qga2luZCA9IHQuc2VuZGVyLnRyYWNrLmtpbmQ7XG4gICAgICAgIHJldHVybiB7XG4gICAgICAgICAgdHlwZToga2luZCxcbiAgICAgICAgICB0cmFuc2NlaXZlcjogdCxcbiAgICAgICAgICBzb3VyY2U6IHN0cmVhbS5zb3VyY2Vba2luZF0sXG4gICAgICAgICAgb3B0aW9uOiB7fSxcbiAgICAgICAgfTtcbiAgICAgIH0pO1xuICAgICAgY29uc3QgaW50ZXJuYWxJZCA9IHRoaXMuX2NyZWF0ZUludGVybmFsSWQoKTtcbiAgICAgIGF3YWl0IHRoaXMuX2NoYWluU2RwUHJvbWlzZShpbnRlcm5hbElkKTsgLy8gQ29waWVkIGZyb20gcHVibGlzaCBtZXRob2QuXG4gICAgICB0aGlzLl9wdWJsaXNoVHJhbnNjZWl2ZXJzLnNldChpbnRlcm5hbElkLCB0cmFuc2NlaXZlckRlc2NyaXB0aW9uKTtcbiAgICAgIGNvbnN0IG9mZmVyPWF3YWl0IHRoaXMucGMuY3JlYXRlT2ZmZXIoKTtcbiAgICAgIGF3YWl0IHRoaXMucGMuc2V0TG9jYWxEZXNjcmlwdGlvbihvZmZlcik7XG4gICAgICBjb25zdCB0cmFja09wdGlvbnMgPSB0cmFuc2NlaXZlcnMubWFwKCh0KSA9PiB7XG4gICAgICAgIGNvbnN0IGtpbmQgPSB0LnNlbmRlci50cmFjay5raW5kO1xuICAgICAgICByZXR1cm4ge1xuICAgICAgICAgIHR5cGU6IGtpbmQsXG4gICAgICAgICAgc291cmNlOiBzdHJlYW0uc291cmNlW2tpbmRdLFxuICAgICAgICAgIG1pZDogdC5taWQsXG4gICAgICAgIH07XG4gICAgICB9KTtcbiAgICAgIGNvbnN0IHB1YmxpY2F0aW9uSWQgPVxuICAgICAgICAgIGF3YWl0IHRoaXMuX3NpZ25hbGluZy5zZW5kU2lnbmFsaW5nTWVzc2FnZSgncHVibGlzaCcsIHtcbiAgICAgICAgICAgIG1lZGlhOiB7dHJhY2tzOiB0cmFja09wdGlvbnN9LFxuICAgICAgICAgICAgYXR0cmlidXRlczogc3RyZWFtLmF0dHJpYnV0ZXMsXG4gICAgICAgICAgICB0cmFuc3BvcnQ6IHtpZDogdGhpcy5faWQsIHR5cGU6ICd3ZWJydGMnfSxcbiAgICAgICAgICB9KS5pZDtcbiAgICAgIHRoaXMuX3B1Ymxpc2hUcmFuc2NlaXZlcnMuZ2V0KGludGVybmFsSWQpLmlkID0gcHVibGljYXRpb25JZDtcbiAgICAgIHRoaXMuX3JldmVyc2VJZE1hcC5zZXQocHVibGljYXRpb25JZCwgaW50ZXJuYWxJZCk7XG4gICAgICBhd2FpdCB0aGlzLl9zaWduYWxpbmcuc2VuZFNpZ25hbGluZ01lc3NhZ2UoXG4gICAgICAgICAgJ3NvYWMnLCB7aWQ6IHRoaXMuX2lkLCBzaWduYWxpbmc6IG9mZmVyfSk7XG4gICAgICByZXR1cm4gbmV3IFByb21pc2UoKHJlc29sdmUsIHJlamVjdCkgPT4ge1xuICAgICAgICB0aGlzLl9wdWJsaXNoUHJvbWlzZXMuc2V0KGludGVybmFsSWQsIHtcbiAgICAgICAgICByZXNvbHZlOiByZXNvbHZlLFxuICAgICAgICAgIHJlamVjdDogcmVqZWN0LFxuICAgICAgICB9KTtcbiAgICAgIH0pO1xuICAgIH1cbiAgfVxuXG4gIGFzeW5jIHB1Ymxpc2goc3RyZWFtLCBvcHRpb25zLCB2aWRlb0NvZGVjcykge1xuICAgIGlmICh0aGlzLl9lbmRlZCkge1xuICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KCdDb25uZWN0aW9uIGNsb3NlZCcpO1xuICAgIH1cbiAgICBpZiAoQXJyYXkuaXNBcnJheShvcHRpb25zKSkge1xuICAgICAgLy8gVGhlIHNlY29uZCBhcmd1bWVudCBpcyBhbiBhcnJheSBvZiBSVENSdHBUcmFuc2NlaXZlcnMuXG4gICAgICByZXR1cm4gdGhpcy5wdWJsaXNoV2l0aFRyYW5zY2VpdmVycyhzdHJlYW0sIG9wdGlvbnMpO1xuICAgIH1cbiAgICBpZiAob3B0aW9ucyA9PT0gdW5kZWZpbmVkKSB7XG4gICAgICBvcHRpb25zID0ge1xuICAgICAgICBhdWRpbzogISFzdHJlYW0ubWVkaWFTdHJlYW0uZ2V0QXVkaW9UcmFja3MoKS5sZW5ndGgsXG4gICAgICAgIHZpZGVvOiAhIXN0cmVhbS5tZWRpYVN0cmVhbS5nZXRWaWRlb1RyYWNrcygpLmxlbmd0aCxcbiAgICAgIH07XG4gICAgfVxuICAgIGlmICh0eXBlb2Ygb3B0aW9ucyAhPT0gJ29iamVjdCcpIHtcbiAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgVHlwZUVycm9yKCdPcHRpb25zIHNob3VsZCBiZSBhbiBvYmplY3QuJykpO1xuICAgIH1cbiAgICBpZiAoKHRoaXMuX2lzUnRwRW5jb2RpbmdQYXJhbWV0ZXJzKG9wdGlvbnMuYXVkaW8pICYmXG4gICAgICAgICB0aGlzLl9pc093dEVuY29kaW5nUGFyYW1ldGVycyhvcHRpb25zLnZpZGVvKSkgfHxcbiAgICAgICAgKHRoaXMuX2lzT3d0RW5jb2RpbmdQYXJhbWV0ZXJzKG9wdGlvbnMuYXVkaW8pICYmXG4gICAgICAgICB0aGlzLl9pc1J0cEVuY29kaW5nUGFyYW1ldGVycyhvcHRpb25zLnZpZGVvKSkpIHtcbiAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgQ29uZmVyZW5jZUVycm9yKFxuICAgICAgICAgICdNaXhpbmcgUlRDUnRwRW5jb2RpbmdQYXJhbWV0ZXJzIGFuZCAnICtcbiAgICAgICAgICAnQXVkaW9FbmNvZGluZ1BhcmFtZXRlcnMvVmlkZW9FbmNvZGluZ1BhcmFtZXRlcnMgaXMgbm90IGFsbG93ZWQuJykpO1xuICAgIH1cbiAgICBpZiAob3B0aW9ucy5hdWRpbyA9PT0gdW5kZWZpbmVkKSB7XG4gICAgICBvcHRpb25zLmF1ZGlvID0gISFzdHJlYW0ubWVkaWFTdHJlYW0uZ2V0QXVkaW9UcmFja3MoKS5sZW5ndGg7XG4gICAgfVxuICAgIGlmIChvcHRpb25zLnZpZGVvID09PSB1bmRlZmluZWQpIHtcbiAgICAgIG9wdGlvbnMudmlkZW8gPSAhIXN0cmVhbS5tZWRpYVN0cmVhbS5nZXRWaWRlb1RyYWNrcygpLmxlbmd0aDtcbiAgICB9XG4gICAgaWYgKCghIW9wdGlvbnMuYXVkaW8gJiYgIXN0cmVhbS5tZWRpYVN0cmVhbS5nZXRBdWRpb1RyYWNrcygpLmxlbmd0aCkgfHxcbiAgICAgICAgKCEhb3B0aW9ucy52aWRlbyAmJiAhc3RyZWFtLm1lZGlhU3RyZWFtLmdldFZpZGVvVHJhY2tzKCkubGVuZ3RoKSkge1xuICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KG5ldyBDb25mZXJlbmNlRXJyb3IoXG4gICAgICAgICAgJ29wdGlvbnMuYXVkaW8vdmlkZW8gaXMgaW5jb25zaXN0ZW50IHdpdGggdHJhY2tzIHByZXNlbnRlZCBpbiB0aGUgJyArXG4gICAgICAgICAgJ01lZGlhU3RyZWFtLicsXG4gICAgICApKTtcbiAgICB9XG4gICAgaWYgKChvcHRpb25zLmF1ZGlvID09PSBmYWxzZSB8fCBvcHRpb25zLmF1ZGlvID09PSBudWxsKSAmJlxuICAgICAgKG9wdGlvbnMudmlkZW8gPT09IGZhbHNlIHx8IG9wdGlvbnMudmlkZW8gPT09IG51bGwpKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IENvbmZlcmVuY2VFcnJvcihcbiAgICAgICAgICAnQ2Fubm90IHB1Ymxpc2ggYSBzdHJlYW0gd2l0aG91dCBhdWRpbyBhbmQgdmlkZW8uJykpO1xuICAgIH1cbiAgICBpZiAodHlwZW9mIG9wdGlvbnMuYXVkaW8gPT09ICdvYmplY3QnKSB7XG4gICAgICBpZiAoIUFycmF5LmlzQXJyYXkob3B0aW9ucy5hdWRpbykpIHtcbiAgICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KG5ldyBUeXBlRXJyb3IoXG4gICAgICAgICAgICAnb3B0aW9ucy5hdWRpbyBzaG91bGQgYmUgYSBib29sZWFuIG9yIGFuIGFycmF5LicpKTtcbiAgICAgIH1cbiAgICAgIGZvciAoY29uc3QgcGFyYW1ldGVycyBvZiBvcHRpb25zLmF1ZGlvKSB7XG4gICAgICAgIGlmICghcGFyYW1ldGVycy5jb2RlYyB8fCB0eXBlb2YgcGFyYW1ldGVycy5jb2RlYy5uYW1lICE9PSAnc3RyaW5nJyB8fCAoXG4gICAgICAgICAgcGFyYW1ldGVycy5tYXhCaXRyYXRlICE9PSB1bmRlZmluZWQgJiYgdHlwZW9mIHBhcmFtZXRlcnMubWF4Qml0cmF0ZVxuICAgICAgICAgICE9PSAnbnVtYmVyJykpIHtcbiAgICAgICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IFR5cGVFcnJvcihcbiAgICAgICAgICAgICAgJ29wdGlvbnMuYXVkaW8gaGFzIGluY29ycmVjdCBwYXJhbWV0ZXJzLicpKTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgIH1cbiAgICBpZiAodHlwZW9mIG9wdGlvbnMudmlkZW8gPT09ICdvYmplY3QnICYmICFBcnJheS5pc0FycmF5KG9wdGlvbnMudmlkZW8pKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IFR5cGVFcnJvcihcbiAgICAgICAgICAnb3B0aW9ucy52aWRlbyBzaG91bGQgYmUgYSBib29sZWFuIG9yIGFuIGFycmF5LicpKTtcbiAgICB9XG4gICAgaWYgKHRoaXMuX2lzT3d0RW5jb2RpbmdQYXJhbWV0ZXJzKG9wdGlvbnMudmlkZW8pKSB7XG4gICAgICBmb3IgKGNvbnN0IHBhcmFtZXRlcnMgb2Ygb3B0aW9ucy52aWRlbykge1xuICAgICAgICBpZiAoIXBhcmFtZXRlcnMuY29kZWMgfHwgdHlwZW9mIHBhcmFtZXRlcnMuY29kZWMubmFtZSAhPT0gJ3N0cmluZycgfHxcbiAgICAgICAgICAoXG4gICAgICAgICAgICBwYXJhbWV0ZXJzLm1heEJpdHJhdGUgIT09IHVuZGVmaW5lZCAmJiB0eXBlb2YgcGFyYW1ldGVyc1xuICAgICAgICAgICAgICAgIC5tYXhCaXRyYXRlICE9PVxuICAgICAgICAgICAgJ251bWJlcicpIHx8IChwYXJhbWV0ZXJzLmNvZGVjLnByb2ZpbGUgIT09IHVuZGVmaW5lZCAmJlxuICAgICAgICAgICAgdHlwZW9mIHBhcmFtZXRlcnMuY29kZWMucHJvZmlsZSAhPT0gJ3N0cmluZycpKSB7XG4gICAgICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KG5ldyBUeXBlRXJyb3IoXG4gICAgICAgICAgICAgICdvcHRpb25zLnZpZGVvIGhhcyBpbmNvcnJlY3QgcGFyYW1ldGVycy4nKSk7XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICB9XG4gICAgY29uc3QgbWVkaWFPcHRpb25zID0ge307XG4gICAgdGhpcy5fY3JlYXRlUGVlckNvbm5lY3Rpb24oKTtcbiAgICBpZiAoc3RyZWFtLm1lZGlhU3RyZWFtLmdldEF1ZGlvVHJhY2tzKCkubGVuZ3RoID4gMCAmJiBvcHRpb25zLmF1ZGlvICE9PVxuICAgICAgZmFsc2UgJiYgb3B0aW9ucy5hdWRpbyAhPT0gbnVsbCkge1xuICAgICAgaWYgKHN0cmVhbS5tZWRpYVN0cmVhbS5nZXRBdWRpb1RyYWNrcygpLmxlbmd0aCA+IDEpIHtcbiAgICAgICAgTG9nZ2VyLndhcm5pbmcoXG4gICAgICAgICAgICAnUHVibGlzaGluZyBhIHN0cmVhbSB3aXRoIG11bHRpcGxlIGF1ZGlvIHRyYWNrcyBpcyBub3QgZnVsbHknXG4gICAgICAgICAgICArICcgc3VwcG9ydGVkLicsXG4gICAgICAgICk7XG4gICAgICB9XG4gICAgICBpZiAodHlwZW9mIG9wdGlvbnMuYXVkaW8gIT09ICdib29sZWFuJyAmJiB0eXBlb2Ygb3B0aW9ucy5hdWRpbyAhPT1cbiAgICAgICAgJ29iamVjdCcpIHtcbiAgICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KG5ldyBDb25mZXJlbmNlRXJyb3IoXG4gICAgICAgICAgICAnVHlwZSBvZiBhdWRpbyBvcHRpb25zIHNob3VsZCBiZSBib29sZWFuIG9yIGFuIG9iamVjdC4nLFxuICAgICAgICApKTtcbiAgICAgIH1cbiAgICAgIG1lZGlhT3B0aW9ucy5hdWRpbyA9IHt9O1xuICAgICAgbWVkaWFPcHRpb25zLmF1ZGlvLnNvdXJjZSA9IHN0cmVhbS5zb3VyY2UuYXVkaW87XG4gICAgfSBlbHNlIHtcbiAgICAgIG1lZGlhT3B0aW9ucy5hdWRpbyA9IGZhbHNlO1xuICAgIH1cbiAgICBpZiAoc3RyZWFtLm1lZGlhU3RyZWFtLmdldFZpZGVvVHJhY2tzKCkubGVuZ3RoID4gMCAmJiBvcHRpb25zLnZpZGVvICE9PVxuICAgICAgZmFsc2UgJiYgb3B0aW9ucy52aWRlbyAhPT0gbnVsbCkge1xuICAgICAgaWYgKHN0cmVhbS5tZWRpYVN0cmVhbS5nZXRWaWRlb1RyYWNrcygpLmxlbmd0aCA+IDEpIHtcbiAgICAgICAgTG9nZ2VyLndhcm5pbmcoXG4gICAgICAgICAgICAnUHVibGlzaGluZyBhIHN0cmVhbSB3aXRoIG11bHRpcGxlIHZpZGVvIHRyYWNrcyBpcyBub3QgZnVsbHkgJ1xuICAgICAgICAgICAgKyAnc3VwcG9ydGVkLicsXG4gICAgICAgICk7XG4gICAgICB9XG4gICAgICBtZWRpYU9wdGlvbnMudmlkZW8gPSB7fTtcbiAgICAgIG1lZGlhT3B0aW9ucy52aWRlby5zb3VyY2UgPSBzdHJlYW0uc291cmNlLnZpZGVvO1xuICAgICAgY29uc3QgdHJhY2tTZXR0aW5ncyA9IHN0cmVhbS5tZWRpYVN0cmVhbS5nZXRWaWRlb1RyYWNrcygpWzBdXG4gICAgICAgICAgLmdldFNldHRpbmdzKCk7XG4gICAgICBtZWRpYU9wdGlvbnMudmlkZW8ucGFyYW1ldGVycyA9IHtcbiAgICAgICAgcmVzb2x1dGlvbjoge1xuICAgICAgICAgIHdpZHRoOiB0cmFja1NldHRpbmdzLndpZHRoLFxuICAgICAgICAgIGhlaWdodDogdHJhY2tTZXR0aW5ncy5oZWlnaHQsXG4gICAgICAgIH0sXG4gICAgICAgIGZyYW1lcmF0ZTogdHJhY2tTZXR0aW5ncy5mcmFtZVJhdGUsXG4gICAgICB9O1xuICAgIH0gZWxzZSB7XG4gICAgICBtZWRpYU9wdGlvbnMudmlkZW8gPSBmYWxzZTtcbiAgICB9XG5cbiAgICBjb25zdCBpbnRlcm5hbElkID0gdGhpcy5fY3JlYXRlSW50ZXJuYWxJZCgpO1xuICAgIC8vIFdhaXRpbmcgZm9yIHByZXZpb3VzIFNEUCBuZWdvdGlhdGlvbiBpZiBuZWVkZWRcbiAgICBhd2FpdCB0aGlzLl9jaGFpblNkcFByb21pc2UoaW50ZXJuYWxJZCk7XG5cbiAgICBjb25zdCBvZmZlck9wdGlvbnMgPSB7fTtcbiAgICBjb25zdCB0cmFuc2NlaXZlcnMgPSBbXTtcbiAgICBpZiAodHlwZW9mIHRoaXMucGMuYWRkVHJhbnNjZWl2ZXIgPT09ICdmdW5jdGlvbicpIHtcbiAgICAgIC8vIHxkaXJlY3Rpb258IHNlZW1zIG5vdCB3b3JraW5nIG9uIFNhZmFyaS5cbiAgICAgIGlmIChtZWRpYU9wdGlvbnMuYXVkaW8gJiYgc3RyZWFtLm1lZGlhU3RyZWFtLmdldEF1ZGlvVHJhY2tzKCkubGVuZ3RoID5cbiAgICAgICAgMCkge1xuICAgICAgICBjb25zdCB0cmFuc2NlaXZlckluaXQgPSB7XG4gICAgICAgICAgZGlyZWN0aW9uOiAnc2VuZG9ubHknLFxuICAgICAgICAgIHN0cmVhbXM6IFtzdHJlYW0ubWVkaWFTdHJlYW1dLFxuICAgICAgICB9O1xuICAgICAgICBpZiAodGhpcy5faXNSdHBFbmNvZGluZ1BhcmFtZXRlcnMob3B0aW9ucy5hdWRpbykpIHtcbiAgICAgICAgICB0cmFuc2NlaXZlckluaXQuc2VuZEVuY29kaW5ncyA9IG9wdGlvbnMuYXVkaW87XG4gICAgICAgIH1cbiAgICAgICAgY29uc3QgdHJhbnNjZWl2ZXIgPSB0aGlzLnBjLmFkZFRyYW5zY2VpdmVyKFxuICAgICAgICAgICAgc3RyZWFtLm1lZGlhU3RyZWFtLmdldEF1ZGlvVHJhY2tzKClbMF0sXG4gICAgICAgICAgICB0cmFuc2NlaXZlckluaXQpO1xuICAgICAgICB0cmFuc2NlaXZlcnMucHVzaCh7XG4gICAgICAgICAgdHlwZTogJ2F1ZGlvJyxcbiAgICAgICAgICB0cmFuc2NlaXZlcixcbiAgICAgICAgICBzb3VyY2U6IG1lZGlhT3B0aW9ucy5hdWRpby5zb3VyY2UsXG4gICAgICAgICAgb3B0aW9uOiB7YXVkaW86IG9wdGlvbnMuYXVkaW99LFxuICAgICAgICB9KTtcblxuICAgICAgICBpZiAoVXRpbHMuaXNGaXJlZm94KCkpIHtcbiAgICAgICAgICAvLyBGaXJlZm94IGRvZXMgbm90IHN1cHBvcnQgZW5jb2RpbmdzIHNldHRpbmcgaW4gYWRkVHJhbnNjZWl2ZXIuXG4gICAgICAgICAgY29uc3QgcGFyYW1ldGVycyA9IHRyYW5zY2VpdmVyLnNlbmRlci5nZXRQYXJhbWV0ZXJzKCk7XG4gICAgICAgICAgcGFyYW1ldGVycy5lbmNvZGluZ3MgPSB0cmFuc2NlaXZlckluaXQuc2VuZEVuY29kaW5ncztcbiAgICAgICAgICBhd2FpdCB0cmFuc2NlaXZlci5zZW5kZXIuc2V0UGFyYW1ldGVycyhwYXJhbWV0ZXJzKTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgICAgaWYgKG1lZGlhT3B0aW9ucy52aWRlbyAmJiBzdHJlYW0ubWVkaWFTdHJlYW0uZ2V0VmlkZW9UcmFja3MoKS5sZW5ndGggPlxuICAgICAgICAwKSB7XG4gICAgICAgIGNvbnN0IHRyYW5zY2VpdmVySW5pdCA9IHtcbiAgICAgICAgICBkaXJlY3Rpb246ICdzZW5kb25seScsXG4gICAgICAgICAgc3RyZWFtczogW3N0cmVhbS5tZWRpYVN0cmVhbV0sXG4gICAgICAgIH07XG4gICAgICAgIGlmICh0aGlzLl9pc1J0cEVuY29kaW5nUGFyYW1ldGVycyhvcHRpb25zLnZpZGVvKSkge1xuICAgICAgICAgIHRyYW5zY2VpdmVySW5pdC5zZW5kRW5jb2RpbmdzID0gb3B0aW9ucy52aWRlbztcbiAgICAgICAgICB0aGlzLl92aWRlb0NvZGVjcyA9IHZpZGVvQ29kZWNzO1xuICAgICAgICB9XG4gICAgICAgIGNvbnN0IHRyYW5zY2VpdmVyID0gdGhpcy5wYy5hZGRUcmFuc2NlaXZlcihcbiAgICAgICAgICAgIHN0cmVhbS5tZWRpYVN0cmVhbS5nZXRWaWRlb1RyYWNrcygpWzBdLFxuICAgICAgICAgICAgdHJhbnNjZWl2ZXJJbml0KTtcbiAgICAgICAgdHJhbnNjZWl2ZXJzLnB1c2goe1xuICAgICAgICAgIHR5cGU6ICd2aWRlbycsXG4gICAgICAgICAgdHJhbnNjZWl2ZXIsXG4gICAgICAgICAgc291cmNlOiBtZWRpYU9wdGlvbnMudmlkZW8uc291cmNlLFxuICAgICAgICAgIG9wdGlvbjoge3ZpZGVvOiBvcHRpb25zLnZpZGVvfSxcbiAgICAgICAgfSk7XG5cbiAgICAgICAgaWYgKFV0aWxzLmlzRmlyZWZveCgpKSB7XG4gICAgICAgICAgLy8gRmlyZWZveCBkb2VzIG5vdCBzdXBwb3J0IGVuY29kaW5ncyBzZXR0aW5nIGluIGFkZFRyYW5zY2VpdmVyLlxuICAgICAgICAgIGNvbnN0IHBhcmFtZXRlcnMgPSB0cmFuc2NlaXZlci5zZW5kZXIuZ2V0UGFyYW1ldGVycygpO1xuICAgICAgICAgIHBhcmFtZXRlcnMuZW5jb2RpbmdzID0gdHJhbnNjZWl2ZXJJbml0LnNlbmRFbmNvZGluZ3M7XG4gICAgICAgICAgYXdhaXQgdHJhbnNjZWl2ZXIuc2VuZGVyLnNldFBhcmFtZXRlcnMocGFyYW1ldGVycyk7XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICB9IGVsc2Uge1xuICAgICAgLy8gU2hvdWxkIG5vdCByZWFjaCBoZXJlXG4gICAgICBpZiAobWVkaWFPcHRpb25zLmF1ZGlvICYmXG4gICAgICAgICAgc3RyZWFtLm1lZGlhU3RyZWFtLmdldEF1ZGlvVHJhY2tzKCkubGVuZ3RoID4gMCkge1xuICAgICAgICBmb3IgKGNvbnN0IHRyYWNrIG9mIHN0cmVhbS5tZWRpYVN0cmVhbS5nZXRBdWRpb1RyYWNrcygpKSB7XG4gICAgICAgICAgdGhpcy5wYy5hZGRUcmFjayh0cmFjaywgc3RyZWFtLm1lZGlhU3RyZWFtKTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgICAgaWYgKG1lZGlhT3B0aW9ucy52aWRlbyAmJlxuICAgICAgICAgIHN0cmVhbS5tZWRpYVN0cmVhbS5nZXRWaWRlb1RyYWNrcygpLmxlbmd0aCA+IDApIHtcbiAgICAgICAgZm9yIChjb25zdCB0cmFjayBvZiBzdHJlYW0ubWVkaWFTdHJlYW0uZ2V0VmlkZW9UcmFja3MoKSkge1xuICAgICAgICAgIHRoaXMucGMuYWRkVHJhY2sodHJhY2ssIHN0cmVhbS5tZWRpYVN0cmVhbSk7XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICAgIG9mZmVyT3B0aW9ucy5vZmZlclRvUmVjZWl2ZUF1ZGlvID0gZmFsc2U7XG4gICAgICBvZmZlck9wdGlvbnMub2ZmZXJUb1JlY2VpdmVWaWRlbyA9IGZhbHNlO1xuICAgIH1cbiAgICB0aGlzLl9wdWJsaXNoVHJhbnNjZWl2ZXJzLnNldChpbnRlcm5hbElkLCB7dHJhbnNjZWl2ZXJzfSk7XG5cbiAgICBsZXQgbG9jYWxEZXNjO1xuICAgIHRoaXMucGMuY3JlYXRlT2ZmZXIob2ZmZXJPcHRpb25zKS50aGVuKChkZXNjKSA9PiB7XG4gICAgICBsb2NhbERlc2MgPSBkZXNjO1xuICAgICAgcmV0dXJuIHRoaXMucGMuc2V0TG9jYWxEZXNjcmlwdGlvbihkZXNjKTtcbiAgICB9KS50aGVuKCgpID0+IHtcbiAgICAgIGNvbnN0IHRyYWNrT3B0aW9ucyA9IFtdO1xuICAgICAgdHJhbnNjZWl2ZXJzLmZvckVhY2goKHt0eXBlLCB0cmFuc2NlaXZlciwgc291cmNlfSkgPT4ge1xuICAgICAgICB0cmFja09wdGlvbnMucHVzaCh7XG4gICAgICAgICAgdHlwZSxcbiAgICAgICAgICBtaWQ6IHRyYW5zY2VpdmVyLm1pZCxcbiAgICAgICAgICBzb3VyY2UsXG4gICAgICAgIH0pO1xuICAgICAgfSk7XG4gICAgICByZXR1cm4gdGhpcy5fc2lnbmFsaW5nLnNlbmRTaWduYWxpbmdNZXNzYWdlKCdwdWJsaXNoJywge1xuICAgICAgICBtZWRpYToge3RyYWNrczogdHJhY2tPcHRpb25zfSxcbiAgICAgICAgYXR0cmlidXRlczogc3RyZWFtLmF0dHJpYnV0ZXMsXG4gICAgICAgIHRyYW5zcG9ydDoge2lkOiB0aGlzLl9pZCwgdHlwZTogJ3dlYnJ0Yyd9LFxuICAgICAgfSkuY2F0Y2goKGUpID0+IHtcbiAgICAgICAgLy8gU2VuZCBTRFAgZXZlbiB3aGVuIGZhaWxlZCB0byBnZXQgQW5zd2VyLlxuICAgICAgICB0aGlzLl9zaWduYWxpbmcuc2VuZFNpZ25hbGluZ01lc3NhZ2UoJ3NvYWMnLCB7XG4gICAgICAgICAgaWQ6IHRoaXMuX2lkLFxuICAgICAgICAgIHNpZ25hbGluZzogbG9jYWxEZXNjLFxuICAgICAgICB9KTtcbiAgICAgICAgdGhyb3cgZTtcbiAgICAgIH0pO1xuICAgIH0pLnRoZW4oKGRhdGEpID0+IHtcbiAgICAgIGNvbnN0IHB1YmxpY2F0aW9uSWQgPSBkYXRhLmlkO1xuICAgICAgY29uc3QgbWVzc2FnZUV2ZW50ID0gbmV3IE1lc3NhZ2VFdmVudCgnaWQnLCB7XG4gICAgICAgIG1lc3NhZ2U6IHB1YmxpY2F0aW9uSWQsXG4gICAgICAgIG9yaWdpbjogdGhpcy5fcmVtb3RlSWQsXG4gICAgICB9KTtcbiAgICAgIHRoaXMuZGlzcGF0Y2hFdmVudChtZXNzYWdlRXZlbnQpO1xuXG4gICAgICB0aGlzLl9wdWJsaXNoVHJhbnNjZWl2ZXJzLmdldChpbnRlcm5hbElkKS5pZCA9IHB1YmxpY2F0aW9uSWQ7XG4gICAgICB0aGlzLl9yZXZlcnNlSWRNYXAuc2V0KHB1YmxpY2F0aW9uSWQsIGludGVybmFsSWQpO1xuXG4gICAgICBpZiAodGhpcy5faWQgJiYgdGhpcy5faWQgIT09IGRhdGEudHJhbnNwb3J0SWQpIHtcbiAgICAgICAgTG9nZ2VyLndhcm5pbmcoJ1NlcnZlciByZXR1cm5zIGNvbmZsaWN0IElEOiAnICsgZGF0YS50cmFuc3BvcnRJZCk7XG4gICAgICB9XG4gICAgICB0aGlzLl9pZCA9IGRhdGEudHJhbnNwb3J0SWQ7XG5cbiAgICAgIC8vIE1vZGlmeSBsb2NhbCBTRFAgYmVmb3JlIHNlbmRpbmdcbiAgICAgIGlmIChvcHRpb25zKSB7XG4gICAgICAgIHRyYW5zY2VpdmVycy5mb3JFYWNoKCh7dHlwZSwgdHJhbnNjZWl2ZXIsIG9wdGlvbn0pID0+IHtcbiAgICAgICAgICBsb2NhbERlc2Muc2RwID0gdGhpcy5fc2V0UnRwUmVjZWl2ZXJPcHRpb25zKFxuICAgICAgICAgICAgICBsb2NhbERlc2Muc2RwLCBvcHRpb24sIHRyYW5zY2VpdmVyLm1pZCk7XG4gICAgICAgICAgbG9jYWxEZXNjLnNkcCA9IHRoaXMuX3NldFJ0cFNlbmRlck9wdGlvbnMoXG4gICAgICAgICAgICAgIGxvY2FsRGVzYy5zZHAsIG9wdGlvbiwgdHJhbnNjZWl2ZXIubWlkKTtcbiAgICAgICAgfSk7XG4gICAgICB9XG4gICAgICB0aGlzLl9zaWduYWxpbmcuc2VuZFNpZ25hbGluZ01lc3NhZ2UoJ3NvYWMnLCB7XG4gICAgICAgIGlkOiB0aGlzLl9pZCxcbiAgICAgICAgc2lnbmFsaW5nOiBsb2NhbERlc2MsXG4gICAgICB9KTtcbiAgICB9KS5jYXRjaCgoZSkgPT4ge1xuICAgICAgTG9nZ2VyLmVycm9yKCdGYWlsZWQgdG8gY3JlYXRlIG9mZmVyIG9yIHNldCBTRFAuIE1lc3NhZ2U6ICdcbiAgICAgICAgICArIGUubWVzc2FnZSk7XG4gICAgICBpZiAodGhpcy5fcHVibGlzaFRyYW5zY2VpdmVycy5nZXQoaW50ZXJuYWxJZCkuaWQpIHtcbiAgICAgICAgdGhpcy5fdW5wdWJsaXNoKGludGVybmFsSWQpO1xuICAgICAgICB0aGlzLl9yZWplY3RQcm9taXNlKGUpO1xuICAgICAgICB0aGlzLl9maXJlRW5kZWRFdmVudE9uUHVibGljYXRpb25PclN1YnNjcmlwdGlvbigpO1xuICAgICAgfSBlbHNlIHtcbiAgICAgICAgdGhpcy5fdW5wdWJsaXNoKGludGVybmFsSWQpO1xuICAgICAgfVxuICAgIH0pO1xuICAgIHJldHVybiBuZXcgUHJvbWlzZSgocmVzb2x2ZSwgcmVqZWN0KSA9PiB7XG4gICAgICB0aGlzLl9wdWJsaXNoUHJvbWlzZXMuc2V0KGludGVybmFsSWQsIHtcbiAgICAgICAgcmVzb2x2ZTogcmVzb2x2ZSxcbiAgICAgICAgcmVqZWN0OiByZWplY3QsXG4gICAgICB9KTtcbiAgICB9KTtcbiAgfVxuXG4gIGFzeW5jIHN1YnNjcmliZShzdHJlYW0sIG9wdGlvbnMpIHtcbiAgICBpZiAodGhpcy5fZW5kZWQpIHtcbiAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdCgnQ29ubmVjdGlvbiBjbG9zZWQnKTtcbiAgICB9XG4gICAgaWYgKG9wdGlvbnMgPT09IHVuZGVmaW5lZCkge1xuICAgICAgb3B0aW9ucyA9IHtcbiAgICAgICAgYXVkaW86ICEhc3RyZWFtLnNldHRpbmdzLmF1ZGlvLFxuICAgICAgICB2aWRlbzogISFzdHJlYW0uc2V0dGluZ3MudmlkZW8sXG4gICAgICB9O1xuICAgIH1cbiAgICBpZiAodHlwZW9mIG9wdGlvbnMgIT09ICdvYmplY3QnKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IFR5cGVFcnJvcignT3B0aW9ucyBzaG91bGQgYmUgYW4gb2JqZWN0LicpKTtcbiAgICB9XG4gICAgaWYgKG9wdGlvbnMuYXVkaW8gPT09IHVuZGVmaW5lZCkge1xuICAgICAgb3B0aW9ucy5hdWRpbyA9ICEhc3RyZWFtLnNldHRpbmdzLmF1ZGlvO1xuICAgIH1cbiAgICBpZiAob3B0aW9ucy52aWRlbyA9PT0gdW5kZWZpbmVkKSB7XG4gICAgICBvcHRpb25zLnZpZGVvID0gISFzdHJlYW0uc2V0dGluZ3MudmlkZW87XG4gICAgfVxuICAgIGlmICgob3B0aW9ucy5hdWRpbyAhPT0gdW5kZWZpbmVkICYmIHR5cGVvZiBvcHRpb25zLmF1ZGlvICE9PSAnb2JqZWN0JyAmJlxuICAgICAgICB0eXBlb2Ygb3B0aW9ucy5hdWRpbyAhPT0gJ2Jvb2xlYW4nICYmIG9wdGlvbnMuYXVkaW8gIT09IG51bGwpIHx8IChcbiAgICAgIG9wdGlvbnMudmlkZW8gIT09IHVuZGVmaW5lZCAmJiB0eXBlb2Ygb3B0aW9ucy52aWRlbyAhPT0gJ29iamVjdCcgJiZcbiAgICAgICAgdHlwZW9mIG9wdGlvbnMudmlkZW8gIT09ICdib29sZWFuJyAmJiBvcHRpb25zLnZpZGVvICE9PSBudWxsKSkge1xuICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KG5ldyBUeXBlRXJyb3IoJ0ludmFsaWQgb3B0aW9ucyB0eXBlLicpKTtcbiAgICB9XG4gICAgaWYgKG9wdGlvbnMuYXVkaW8gJiYgIXN0cmVhbS5zZXR0aW5ncy5hdWRpbyB8fCAob3B0aW9ucy52aWRlbyAmJlxuICAgICAgICAhc3RyZWFtLnNldHRpbmdzLnZpZGVvKSkge1xuICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KG5ldyBDb25mZXJlbmNlRXJyb3IoXG4gICAgICAgICAgJ29wdGlvbnMuYXVkaW8vdmlkZW8gY2Fubm90IGJlIHRydWUgb3IgYW4gb2JqZWN0IGlmIHRoZXJlIGlzIG5vICdcbiAgICAgICAgICArICdhdWRpby92aWRlbyB0cmFjayBpbiByZW1vdGUgc3RyZWFtLicsXG4gICAgICApKTtcbiAgICB9XG4gICAgaWYgKG9wdGlvbnMuYXVkaW8gPT09IGZhbHNlICYmIG9wdGlvbnMudmlkZW8gPT09IGZhbHNlKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IENvbmZlcmVuY2VFcnJvcihcbiAgICAgICAgICAnQ2Fubm90IHN1YnNjcmliZSBhIHN0cmVhbSB3aXRob3V0IGF1ZGlvIGFuZCB2aWRlby4nKSk7XG4gICAgfVxuICAgIGNvbnN0IG1lZGlhT3B0aW9ucyA9IHt9O1xuICAgIGlmIChvcHRpb25zLmF1ZGlvKSB7XG4gICAgICBpZiAodHlwZW9mIG9wdGlvbnMuYXVkaW8gPT09ICdvYmplY3QnICYmXG4gICAgICAgICAgQXJyYXkuaXNBcnJheShvcHRpb25zLmF1ZGlvLmNvZGVjcykpIHtcbiAgICAgICAgaWYgKG9wdGlvbnMuYXVkaW8uY29kZWNzLmxlbmd0aCA9PT0gMCkge1xuICAgICAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgVHlwZUVycm9yKFxuICAgICAgICAgICAgICAnQXVkaW8gY29kZWMgY2Fubm90IGJlIGFuIGVtcHR5IGFycmF5LicpKTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgICAgbWVkaWFPcHRpb25zLmF1ZGlvID0ge307XG4gICAgICBtZWRpYU9wdGlvbnMuYXVkaW8uZnJvbSA9IHN0cmVhbS5pZDtcbiAgICB9IGVsc2Uge1xuICAgICAgbWVkaWFPcHRpb25zLmF1ZGlvID0gZmFsc2U7XG4gICAgfVxuICAgIGlmIChvcHRpb25zLnZpZGVvKSB7XG4gICAgICBpZiAodHlwZW9mIG9wdGlvbnMudmlkZW8gPT09ICdvYmplY3QnICYmXG4gICAgICAgICAgQXJyYXkuaXNBcnJheShvcHRpb25zLnZpZGVvLmNvZGVjcykpIHtcbiAgICAgICAgaWYgKG9wdGlvbnMudmlkZW8uY29kZWNzLmxlbmd0aCA9PT0gMCkge1xuICAgICAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgVHlwZUVycm9yKFxuICAgICAgICAgICAgICAnVmlkZW8gY29kZWMgY2Fubm90IGJlIGFuIGVtcHR5IGFycmF5LicpKTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgICAgbWVkaWFPcHRpb25zLnZpZGVvID0ge307XG4gICAgICBtZWRpYU9wdGlvbnMudmlkZW8uZnJvbSA9IHN0cmVhbS5pZDtcbiAgICAgIGlmIChvcHRpb25zLnZpZGVvLnJlc29sdXRpb24gfHwgb3B0aW9ucy52aWRlby5mcmFtZVJhdGUgfHwgKG9wdGlvbnMudmlkZW9cbiAgICAgICAgICAuYml0cmF0ZU11bHRpcGxpZXIgJiYgb3B0aW9ucy52aWRlby5iaXRyYXRlTXVsdGlwbGllciAhPT0gMSkgfHxcbiAgICAgICAgb3B0aW9ucy52aWRlby5rZXlGcmFtZUludGVydmFsKSB7XG4gICAgICAgIG1lZGlhT3B0aW9ucy52aWRlby5wYXJhbWV0ZXJzID0ge1xuICAgICAgICAgIHJlc29sdXRpb246IG9wdGlvbnMudmlkZW8ucmVzb2x1dGlvbixcbiAgICAgICAgICBmcmFtZXJhdGU6IG9wdGlvbnMudmlkZW8uZnJhbWVSYXRlLFxuICAgICAgICAgIGJpdHJhdGU6IG9wdGlvbnMudmlkZW8uYml0cmF0ZU11bHRpcGxpZXIgPyAneCdcbiAgICAgICAgICAgICAgKyBvcHRpb25zLnZpZGVvLmJpdHJhdGVNdWx0aXBsaWVyLnRvU3RyaW5nKCkgOiB1bmRlZmluZWQsXG4gICAgICAgICAga2V5RnJhbWVJbnRlcnZhbDogb3B0aW9ucy52aWRlby5rZXlGcmFtZUludGVydmFsLFxuICAgICAgICB9O1xuICAgICAgfVxuICAgICAgaWYgKG9wdGlvbnMudmlkZW8ucmlkKSB7XG4gICAgICAgIC8vIFVzZSByaWQgbWF0Y2hlZCB0cmFjayBJRCBhcyBmcm9tIGlmIHBvc3NpYmxlXG4gICAgICAgIGNvbnN0IG1hdGNoZWRTZXR0aW5nID0gc3RyZWFtLnNldHRpbmdzLnZpZGVvXG4gICAgICAgICAgICAuZmluZCgodmlkZW8pID0+IHZpZGVvLnJpZCA9PT0gb3B0aW9ucy52aWRlby5yaWQpO1xuICAgICAgICBpZiAobWF0Y2hlZFNldHRpbmcgJiYgbWF0Y2hlZFNldHRpbmcuX3RyYWNrSWQpIHtcbiAgICAgICAgICBtZWRpYU9wdGlvbnMudmlkZW8uZnJvbSA9IG1hdGNoZWRTZXR0aW5nLl90cmFja0lkO1xuICAgICAgICAgIC8vIElnbm9yZSBvdGhlciBzZXR0aW5ncyB3aGVuIFJJRCBzZXQuXG4gICAgICAgICAgZGVsZXRlIG1lZGlhT3B0aW9ucy52aWRlby5wYXJhbWV0ZXJzO1xuICAgICAgICB9XG4gICAgICAgIG9wdGlvbnMudmlkZW8gPSB0cnVlO1xuICAgICAgfVxuICAgIH0gZWxzZSB7XG4gICAgICBtZWRpYU9wdGlvbnMudmlkZW8gPSBmYWxzZTtcbiAgICB9XG5cbiAgICBjb25zdCBpbnRlcm5hbElkID0gdGhpcy5fY3JlYXRlSW50ZXJuYWxJZCgpO1xuICAgIC8vIFdhaXRpbmcgZm9yIHByZXZpb3VzIFNEUCBuZWdvdGlhdGlvbiBpZiBuZWVkZWRcbiAgICBhd2FpdCB0aGlzLl9jaGFpblNkcFByb21pc2UoaW50ZXJuYWxJZCk7XG5cbiAgICBjb25zdCBvZmZlck9wdGlvbnMgPSB7fTtcbiAgICBjb25zdCB0cmFuc2NlaXZlcnMgPSBbXTtcbiAgICB0aGlzLl9jcmVhdGVQZWVyQ29ubmVjdGlvbigpO1xuICAgIGlmICh0eXBlb2YgdGhpcy5wYy5hZGRUcmFuc2NlaXZlciA9PT0gJ2Z1bmN0aW9uJykge1xuICAgICAgLy8gfGRpcmVjdGlvbnwgc2VlbXMgbm90IHdvcmtpbmcgb24gU2FmYXJpLlxuICAgICAgaWYgKG1lZGlhT3B0aW9ucy5hdWRpbykge1xuICAgICAgICBjb25zdCB0cmFuc2NlaXZlciA9IHRoaXMucGMuYWRkVHJhbnNjZWl2ZXIoXG4gICAgICAgICAgICAnYXVkaW8nLCB7ZGlyZWN0aW9uOiAncmVjdm9ubHknfSk7XG4gICAgICAgIHRyYW5zY2VpdmVycy5wdXNoKHtcbiAgICAgICAgICB0eXBlOiAnYXVkaW8nLFxuICAgICAgICAgIHRyYW5zY2VpdmVyLFxuICAgICAgICAgIGZyb206IG1lZGlhT3B0aW9ucy5hdWRpby5mcm9tLFxuICAgICAgICAgIG9wdGlvbjoge2F1ZGlvOiBvcHRpb25zLmF1ZGlvfSxcbiAgICAgICAgfSk7XG4gICAgICB9XG4gICAgICBpZiAobWVkaWFPcHRpb25zLnZpZGVvKSB7XG4gICAgICAgIGNvbnN0IHRyYW5zY2VpdmVyID0gdGhpcy5wYy5hZGRUcmFuc2NlaXZlcihcbiAgICAgICAgICAgICd2aWRlbycsIHtkaXJlY3Rpb246ICdyZWN2b25seSd9KTtcbiAgICAgICAgdHJhbnNjZWl2ZXJzLnB1c2goe1xuICAgICAgICAgIHR5cGU6ICd2aWRlbycsXG4gICAgICAgICAgdHJhbnNjZWl2ZXIsXG4gICAgICAgICAgZnJvbTogbWVkaWFPcHRpb25zLnZpZGVvLmZyb20sXG4gICAgICAgICAgcGFyYW1ldGVyczogbWVkaWFPcHRpb25zLnZpZGVvLnBhcmFtZXRlcnMsXG4gICAgICAgICAgb3B0aW9uOiB7dmlkZW86IG9wdGlvbnMudmlkZW99LFxuICAgICAgICB9KTtcbiAgICAgIH1cbiAgICB9IGVsc2Uge1xuICAgICAgb2ZmZXJPcHRpb25zLm9mZmVyVG9SZWNlaXZlQXVkaW8gPSAhIW9wdGlvbnMuYXVkaW87XG4gICAgICBvZmZlck9wdGlvbnMub2ZmZXJUb1JlY2VpdmVWaWRlbyA9ICEhb3B0aW9ucy52aWRlbztcbiAgICB9XG4gICAgdGhpcy5fc3Vic2NyaWJlVHJhbnNjZWl2ZXJzLnNldChpbnRlcm5hbElkLCB7dHJhbnNjZWl2ZXJzfSk7XG5cbiAgICBsZXQgbG9jYWxEZXNjO1xuICAgIHRoaXMucGMuY3JlYXRlT2ZmZXIob2ZmZXJPcHRpb25zKS50aGVuKChkZXNjKSA9PiB7XG4gICAgICBsb2NhbERlc2MgPSBkZXNjO1xuICAgICAgcmV0dXJuIHRoaXMucGMuc2V0TG9jYWxEZXNjcmlwdGlvbihkZXNjKVxuICAgICAgICAgIC5jYXRjaCgoZXJyb3JNZXNzYWdlKSA9PiB7XG4gICAgICAgICAgICBMb2dnZXIuZXJyb3IoJ1NldCBsb2NhbCBkZXNjcmlwdGlvbiBmYWlsZWQuIE1lc3NhZ2U6ICcgK1xuICAgICAgICAgICAgICAgIEpTT04uc3RyaW5naWZ5KGVycm9yTWVzc2FnZSkpO1xuICAgICAgICAgICAgdGhyb3cgZXJyb3JNZXNzYWdlO1xuICAgICAgICAgIH0pO1xuICAgIH0sIGZ1bmN0aW9uKGVycm9yKSB7XG4gICAgICBMb2dnZXIuZXJyb3IoJ0NyZWF0ZSBvZmZlciBmYWlsZWQuIEVycm9yIGluZm86ICcgKyBKU09OLnN0cmluZ2lmeShcbiAgICAgICAgICBlcnJvcikpO1xuICAgICAgdGhyb3cgZXJyb3I7XG4gICAgfSkudGhlbigoKSA9PiB7XG4gICAgICBjb25zdCB0cmFja09wdGlvbnMgPSBbXTtcbiAgICAgIHRyYW5zY2VpdmVycy5mb3JFYWNoKCh7dHlwZSwgdHJhbnNjZWl2ZXIsIGZyb20sIHBhcmFtZXRlcnMsIG9wdGlvbn0pID0+IHtcbiAgICAgICAgdHJhY2tPcHRpb25zLnB1c2goe1xuICAgICAgICAgIHR5cGUsXG4gICAgICAgICAgbWlkOiB0cmFuc2NlaXZlci5taWQsXG4gICAgICAgICAgZnJvbTogZnJvbSxcbiAgICAgICAgICBwYXJhbWV0ZXJzOiBwYXJhbWV0ZXJzLFxuICAgICAgICB9KTtcbiAgICAgIH0pO1xuICAgICAgcmV0dXJuIHRoaXMuX3NpZ25hbGluZy5zZW5kU2lnbmFsaW5nTWVzc2FnZSgnc3Vic2NyaWJlJywge1xuICAgICAgICBtZWRpYToge3RyYWNrczogdHJhY2tPcHRpb25zfSxcbiAgICAgICAgdHJhbnNwb3J0OiB7aWQ6IHRoaXMuX2lkLCB0eXBlOiAnd2VicnRjJ30sXG4gICAgICB9KS5jYXRjaCgoZSkgPT4ge1xuICAgICAgICAvLyBTZW5kIFNEUCBldmVuIHdoZW4gZmFpbGVkIHRvIGdldCBBbnN3ZXIuXG4gICAgICAgIHRoaXMuX3NpZ25hbGluZy5zZW5kU2lnbmFsaW5nTWVzc2FnZSgnc29hYycsIHtcbiAgICAgICAgICBpZDogdGhpcy5faWQsXG4gICAgICAgICAgc2lnbmFsaW5nOiBsb2NhbERlc2MsXG4gICAgICAgIH0pO1xuICAgICAgICB0aHJvdyBlO1xuICAgICAgfSk7XG4gICAgfSkudGhlbigoZGF0YSkgPT4ge1xuICAgICAgY29uc3Qgc3Vic2NyaXB0aW9uSWQgPSBkYXRhLmlkO1xuICAgICAgY29uc3QgbWVzc2FnZUV2ZW50ID0gbmV3IE1lc3NhZ2VFdmVudCgnaWQnLCB7XG4gICAgICAgIG1lc3NhZ2U6IHN1YnNjcmlwdGlvbklkLFxuICAgICAgICBvcmlnaW46IHRoaXMuX3JlbW90ZUlkLFxuICAgICAgfSk7XG4gICAgICB0aGlzLmRpc3BhdGNoRXZlbnQobWVzc2FnZUV2ZW50KTtcblxuICAgICAgdGhpcy5fc3Vic2NyaWJlVHJhbnNjZWl2ZXJzLmdldChpbnRlcm5hbElkKS5pZCA9IHN1YnNjcmlwdGlvbklkO1xuICAgICAgdGhpcy5fcmV2ZXJzZUlkTWFwLnNldChzdWJzY3JpcHRpb25JZCwgaW50ZXJuYWxJZCk7XG4gICAgICBpZiAodGhpcy5faWQgJiYgdGhpcy5faWQgIT09IGRhdGEudHJhbnNwb3J0SWQpIHtcbiAgICAgICAgTG9nZ2VyLndhcm5pbmcoJ1NlcnZlciByZXR1cm5zIGNvbmZsaWN0IElEOiAnICsgZGF0YS50cmFuc3BvcnRJZCk7XG4gICAgICB9XG4gICAgICB0aGlzLl9pZCA9IGRhdGEudHJhbnNwb3J0SWQ7XG5cbiAgICAgIC8vIE1vZGlmeSBsb2NhbCBTRFAgYmVmb3JlIHNlbmRpbmdcbiAgICAgIGlmIChvcHRpb25zKSB7XG4gICAgICAgIHRyYW5zY2VpdmVycy5mb3JFYWNoKCh7dHlwZSwgdHJhbnNjZWl2ZXIsIG9wdGlvbn0pID0+IHtcbiAgICAgICAgICBsb2NhbERlc2Muc2RwID0gdGhpcy5fc2V0UnRwUmVjZWl2ZXJPcHRpb25zKFxuICAgICAgICAgICAgICBsb2NhbERlc2Muc2RwLCBvcHRpb24sIHRyYW5zY2VpdmVyLm1pZCk7XG4gICAgICAgIH0pO1xuICAgICAgfVxuICAgICAgdGhpcy5fc2lnbmFsaW5nLnNlbmRTaWduYWxpbmdNZXNzYWdlKCdzb2FjJywge1xuICAgICAgICBpZDogdGhpcy5faWQsXG4gICAgICAgIHNpZ25hbGluZzogbG9jYWxEZXNjLFxuICAgICAgfSk7XG4gICAgfSkuY2F0Y2goKGUpID0+IHtcbiAgICAgIExvZ2dlci5lcnJvcignRmFpbGVkIHRvIGNyZWF0ZSBvZmZlciBvciBzZXQgU0RQLiBNZXNzYWdlOiAnXG4gICAgICAgICAgKyBlLm1lc3NhZ2UpO1xuICAgICAgaWYgKHRoaXMuX3N1YnNjcmliZVRyYW5zY2VpdmVycy5nZXQoaW50ZXJuYWxJZCkuaWQpIHtcbiAgICAgICAgdGhpcy5fdW5zdWJzY3JpYmUoaW50ZXJuYWxJZCk7XG4gICAgICAgIHRoaXMuX3JlamVjdFByb21pc2UoZSk7XG4gICAgICAgIHRoaXMuX2ZpcmVFbmRlZEV2ZW50T25QdWJsaWNhdGlvbk9yU3Vic2NyaXB0aW9uKCk7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICB0aGlzLl91bnN1YnNjcmliZShpbnRlcm5hbElkKTtcbiAgICAgIH1cbiAgICB9KTtcbiAgICByZXR1cm4gbmV3IFByb21pc2UoKHJlc29sdmUsIHJlamVjdCkgPT4ge1xuICAgICAgdGhpcy5fc3Vic2NyaWJlUHJvbWlzZXMuc2V0KGludGVybmFsSWQsIHtcbiAgICAgICAgcmVzb2x2ZTogcmVzb2x2ZSxcbiAgICAgICAgcmVqZWN0OiByZWplY3QsXG4gICAgICB9KTtcbiAgICB9KTtcbiAgfVxuXG4gIGNsb3NlKCkge1xuICAgIGlmICh0aGlzLnBjICYmIHRoaXMucGMuc2lnbmFsaW5nU3RhdGUgIT09ICdjbG9zZWQnKSB7XG4gICAgICB0aGlzLnBjLmNsb3NlKCk7XG4gICAgfVxuICB9XG5cbiAgX2NoYWluU2RwUHJvbWlzZShpbnRlcm5hbElkKSB7XG4gICAgY29uc3QgcHJpb3IgPSB0aGlzLl9zZHBQcm9taXNlO1xuICAgIGNvbnN0IG5lZ290aWF0aW9uVGltZW91dCA9IDEwMDAwO1xuICAgIHRoaXMuX3NkcFByb21pc2UgPSBwcmlvci50aGVuKFxuICAgICAgICAoKSA9PiBuZXcgUHJvbWlzZSgocmVzb2x2ZSwgcmVqZWN0KSA9PiB7XG4gICAgICAgICAgY29uc3QgcmVzb2x2ZXIgPSB7ZmluaXNoOiBmYWxzZSwgcmVzb2x2ZSwgcmVqZWN0fTtcbiAgICAgICAgICB0aGlzLl9zZHBSZXNvbHZlcnMucHVzaChyZXNvbHZlcik7XG4gICAgICAgICAgdGhpcy5fc2RwUmVzb2x2ZXJNYXAuc2V0KGludGVybmFsSWQsIHJlc29sdmVyKTtcbiAgICAgICAgICBzZXRUaW1lb3V0KCgpID0+IHJlamVjdCgnVGltZW91dCB0byBnZXQgU0RQIGFuc3dlcicpLFxuICAgICAgICAgICAgICBuZWdvdGlhdGlvblRpbWVvdXQpO1xuICAgICAgICB9KSk7XG4gICAgcmV0dXJuIHByaW9yLmNhdGNoKChlKT0+e1xuICAgICAgLy9cbiAgICB9KTtcbiAgfVxuXG4gIF9uZXh0U2RwUHJvbWlzZSgpIHtcbiAgICBsZXQgcmV0ID0gZmFsc2U7XG4gICAgLy8gU2tpcCB0aGUgZmluaXNoZWQgc2RwIHByb21pc2VcbiAgICB3aGlsZSAodGhpcy5fc2RwUmVzb2x2ZU51bSA8IHRoaXMuX3NkcFJlc29sdmVycy5sZW5ndGgpIHtcbiAgICAgIGNvbnN0IHJlc29sdmVyID0gdGhpcy5fc2RwUmVzb2x2ZXJzW3RoaXMuX3NkcFJlc29sdmVOdW1dO1xuICAgICAgdGhpcy5fc2RwUmVzb2x2ZU51bSsrO1xuICAgICAgaWYgKCFyZXNvbHZlci5maW5pc2gpIHtcbiAgICAgICAgcmVzb2x2ZXIucmVzb2x2ZSgpO1xuICAgICAgICByZXNvbHZlci5maW5pc2ggPSB0cnVlO1xuICAgICAgICByZXQgPSB0cnVlO1xuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gcmV0O1xuICB9XG5cbiAgX2NyZWF0ZUludGVybmFsSWQoKSB7XG4gICAgcmV0dXJuIHRoaXMuX2ludGVybmFsQ291bnQrKztcbiAgfVxuXG4gIF91bnB1Ymxpc2goaW50ZXJuYWxJZCkge1xuICAgIGlmICh0aGlzLl9wdWJsaXNoVHJhbnNjZWl2ZXJzLmhhcyhpbnRlcm5hbElkKSkge1xuICAgICAgY29uc3Qge2lkLCB0cmFuc2NlaXZlcnN9ID0gdGhpcy5fcHVibGlzaFRyYW5zY2VpdmVycy5nZXQoaW50ZXJuYWxJZCk7XG4gICAgICBpZiAoaWQpIHtcbiAgICAgICAgdGhpcy5fc2lnbmFsaW5nLnNlbmRTaWduYWxpbmdNZXNzYWdlKCd1bnB1Ymxpc2gnLCB7aWR9KVxuICAgICAgICAgICAgLmNhdGNoKChlKSA9PiB7XG4gICAgICAgICAgICAgIExvZ2dlci53YXJuaW5nKCdNQ1UgcmV0dXJucyBuZWdhdGl2ZSBhY2sgZm9yIHVucHVibGlzaGluZywgJyArIGUpO1xuICAgICAgICAgICAgfSk7XG4gICAgICAgIHRoaXMuX3JldmVyc2VJZE1hcC5kZWxldGUoaWQpO1xuICAgICAgfVxuICAgICAgLy8gQ2xlYW4gdHJhbnNjZWl2ZXJcbiAgICAgIHRyYW5zY2VpdmVycy5mb3JFYWNoKCh7dHJhbnNjZWl2ZXJ9KSA9PiB7XG4gICAgICAgIGlmICh0aGlzLnBjLnNpZ25hbGluZ1N0YXRlID09PSAnc3RhYmxlJykge1xuICAgICAgICAgIHRyYW5zY2VpdmVyLnNlbmRlci5yZXBsYWNlVHJhY2sobnVsbCk7XG4gICAgICAgICAgdGhpcy5wYy5yZW1vdmVUcmFjayh0cmFuc2NlaXZlci5zZW5kZXIpO1xuICAgICAgICB9XG4gICAgICB9KTtcbiAgICAgIHRoaXMuX3B1Ymxpc2hUcmFuc2NlaXZlcnMuZGVsZXRlKGludGVybmFsSWQpO1xuICAgICAgLy8gRmlyZSBlbmRlZCBldmVudFxuICAgICAgaWYgKHRoaXMuX3B1YmxpY2F0aW9ucy5oYXMoaWQpKSB7XG4gICAgICAgIGNvbnN0IGV2ZW50ID0gbmV3IE93dEV2ZW50KCdlbmRlZCcpO1xuICAgICAgICB0aGlzLl9wdWJsaWNhdGlvbnMuZ2V0KGlkKS5kaXNwYXRjaEV2ZW50KGV2ZW50KTtcbiAgICAgICAgdGhpcy5fcHVibGljYXRpb25zLmRlbGV0ZShpZCk7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICBMb2dnZXIud2FybmluZygnSW52YWxpZCBwdWJsaWNhdGlvbiB0byB1bnB1Ymxpc2g6ICcgKyBpZCk7XG4gICAgICAgIGlmICh0aGlzLl9wdWJsaXNoUHJvbWlzZXMuaGFzKGludGVybmFsSWQpKSB7XG4gICAgICAgICAgdGhpcy5fcHVibGlzaFByb21pc2VzLmdldChpbnRlcm5hbElkKS5yZWplY3QoXG4gICAgICAgICAgICAgIG5ldyBDb25mZXJlbmNlRXJyb3IoJ0ZhaWxlZCB0byBwdWJsaXNoJykpO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgICBpZiAodGhpcy5fc2RwUmVzb2x2ZXJNYXAuaGFzKGludGVybmFsSWQpKSB7XG4gICAgICAgIGNvbnN0IHJlc29sdmVyID0gdGhpcy5fc2RwUmVzb2x2ZXJNYXAuZ2V0KGludGVybmFsSWQpO1xuICAgICAgICBpZiAoIXJlc29sdmVyLmZpbmlzaCkge1xuICAgICAgICAgIHJlc29sdmVyLnJlc29sdmUoKTtcbiAgICAgICAgICByZXNvbHZlci5maW5pc2ggPSB0cnVlO1xuICAgICAgICB9XG4gICAgICAgIHRoaXMuX3NkcFJlc29sdmVyTWFwLmRlbGV0ZShpbnRlcm5hbElkKTtcbiAgICAgIH1cbiAgICAgIC8vIENyZWF0ZSBvZmZlciwgc2V0IGxvY2FsIGFuZCByZW1vdGUgZGVzY3JpcHRpb25cbiAgICB9XG4gIH1cblxuICBfdW5zdWJzY3JpYmUoaW50ZXJuYWxJZCkge1xuICAgIGlmICh0aGlzLl9zdWJzY3JpYmVUcmFuc2NlaXZlcnMuaGFzKGludGVybmFsSWQpKSB7XG4gICAgICBjb25zdCB7aWQsIHRyYW5zY2VpdmVyc30gPSB0aGlzLl9zdWJzY3JpYmVUcmFuc2NlaXZlcnMuZ2V0KGludGVybmFsSWQpO1xuICAgICAgaWYgKGlkKSB7XG4gICAgICAgIHRoaXMuX3NpZ25hbGluZy5zZW5kU2lnbmFsaW5nTWVzc2FnZSgndW5zdWJzY3JpYmUnLCB7aWR9KVxuICAgICAgICAgICAgLmNhdGNoKChlKSA9PiB7XG4gICAgICAgICAgICAgIExvZ2dlci53YXJuaW5nKFxuICAgICAgICAgICAgICAgICAgJ01DVSByZXR1cm5zIG5lZ2F0aXZlIGFjayBmb3IgdW5zdWJzY3JpYmluZywgJyArIGUpO1xuICAgICAgICAgICAgfSk7XG4gICAgICB9XG4gICAgICAvLyBDbGVhbiB0cmFuc2NlaXZlclxuICAgICAgdHJhbnNjZWl2ZXJzLmZvckVhY2goKHt0cmFuc2NlaXZlcn0pID0+IHtcbiAgICAgICAgdHJhbnNjZWl2ZXIucmVjZWl2ZXIudHJhY2suc3RvcCgpO1xuICAgICAgfSk7XG4gICAgICB0aGlzLl9zdWJzY3JpYmVUcmFuc2NlaXZlcnMuZGVsZXRlKGludGVybmFsSWQpO1xuICAgICAgLy8gRmlyZSBlbmRlZCBldmVudFxuICAgICAgaWYgKHRoaXMuX3N1YnNjcmlwdGlvbnMuaGFzKGlkKSkge1xuICAgICAgICBjb25zdCBldmVudCA9IG5ldyBPd3RFdmVudCgnZW5kZWQnKTtcbiAgICAgICAgdGhpcy5fc3Vic2NyaXB0aW9ucy5nZXQoaWQpLmRpc3BhdGNoRXZlbnQoZXZlbnQpO1xuICAgICAgICB0aGlzLl9zdWJzY3JpcHRpb25zLmRlbGV0ZShpZCk7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICBMb2dnZXIud2FybmluZygnSW52YWxpZCBzdWJzY3JpcHRpb24gdG8gdW5zdWJzY3JpYmU6ICcgKyBpZCk7XG4gICAgICAgIGlmICh0aGlzLl9zdWJzY3JpYmVQcm9taXNlcy5oYXMoaW50ZXJuYWxJZCkpIHtcbiAgICAgICAgICB0aGlzLl9zdWJzY3JpYmVQcm9taXNlcy5nZXQoaW50ZXJuYWxJZCkucmVqZWN0KFxuICAgICAgICAgICAgICBuZXcgQ29uZmVyZW5jZUVycm9yKCdGYWlsZWQgdG8gc3Vic2NyaWJlJykpO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgICBpZiAodGhpcy5fc2RwUmVzb2x2ZXJNYXAuaGFzKGludGVybmFsSWQpKSB7XG4gICAgICAgIGNvbnN0IHJlc29sdmVyID0gdGhpcy5fc2RwUmVzb2x2ZXJNYXAuZ2V0KGludGVybmFsSWQpO1xuICAgICAgICBpZiAoIXJlc29sdmVyLmZpbmlzaCkge1xuICAgICAgICAgIHJlc29sdmVyLnJlc29sdmUoKTtcbiAgICAgICAgICByZXNvbHZlci5maW5pc2ggPSB0cnVlO1xuICAgICAgICB9XG4gICAgICAgIHRoaXMuX3NkcFJlc29sdmVyTWFwLmRlbGV0ZShpbnRlcm5hbElkKTtcbiAgICAgIH1cbiAgICAgIC8vIERpc2FibGUgbWVkaWEgaW4gcmVtb3RlIFNEUFxuICAgICAgLy8gU2V0IHJlbW90ZURlc2NyaXB0aW9uIGFuZCBzZXQgbG9jYWxEZXNjcmlwdGlvblxuICAgIH1cbiAgfVxuXG4gIF9tdXRlT3JVbm11dGUoc2Vzc2lvbklkLCBpc011dGUsIGlzUHViLCB0cmFja0tpbmQpIHtcbiAgICBjb25zdCBldmVudE5hbWUgPSBpc1B1YiA/ICdzdHJlYW0tY29udHJvbCcgOlxuICAgICAgJ3N1YnNjcmlwdGlvbi1jb250cm9sJztcbiAgICBjb25zdCBvcGVyYXRpb24gPSBpc011dGUgPyAncGF1c2UnIDogJ3BsYXknO1xuICAgIHJldHVybiB0aGlzLl9zaWduYWxpbmcuc2VuZFNpZ25hbGluZ01lc3NhZ2UoZXZlbnROYW1lLCB7XG4gICAgICBpZDogc2Vzc2lvbklkLFxuICAgICAgb3BlcmF0aW9uOiBvcGVyYXRpb24sXG4gICAgICBkYXRhOiB0cmFja0tpbmQsXG4gICAgfSkudGhlbigoKSA9PiB7XG4gICAgICBpZiAoIWlzUHViKSB7XG4gICAgICAgIGNvbnN0IG11dGVFdmVudE5hbWUgPSBpc011dGUgPyAnbXV0ZScgOiAndW5tdXRlJztcbiAgICAgICAgdGhpcy5fc3Vic2NyaXB0aW9ucy5nZXQoc2Vzc2lvbklkKS5kaXNwYXRjaEV2ZW50KFxuICAgICAgICAgICAgbmV3IE11dGVFdmVudChtdXRlRXZlbnROYW1lLCB7a2luZDogdHJhY2tLaW5kfSkpO1xuICAgICAgfVxuICAgIH0pO1xuICB9XG5cbiAgX2FwcGx5T3B0aW9ucyhzZXNzaW9uSWQsIG9wdGlvbnMpIHtcbiAgICBpZiAodHlwZW9mIG9wdGlvbnMgIT09ICdvYmplY3QnIHx8IHR5cGVvZiBvcHRpb25zLnZpZGVvICE9PSAnb2JqZWN0Jykge1xuICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KG5ldyBDb25mZXJlbmNlRXJyb3IoXG4gICAgICAgICAgJ09wdGlvbnMgc2hvdWxkIGJlIGFuIG9iamVjdC4nKSk7XG4gICAgfVxuICAgIGNvbnN0IHZpZGVvT3B0aW9ucyA9IHt9O1xuICAgIHZpZGVvT3B0aW9ucy5yZXNvbHV0aW9uID0gb3B0aW9ucy52aWRlby5yZXNvbHV0aW9uO1xuICAgIHZpZGVvT3B0aW9ucy5mcmFtZXJhdGUgPSBvcHRpb25zLnZpZGVvLmZyYW1lUmF0ZTtcbiAgICB2aWRlb09wdGlvbnMuYml0cmF0ZSA9IG9wdGlvbnMudmlkZW8uYml0cmF0ZU11bHRpcGxpZXIgPyAneCcgKyBvcHRpb25zLnZpZGVvXG4gICAgICAgIC5iaXRyYXRlTXVsdGlwbGllclxuICAgICAgICAudG9TdHJpbmcoKSA6IHVuZGVmaW5lZDtcbiAgICB2aWRlb09wdGlvbnMua2V5RnJhbWVJbnRlcnZhbCA9IG9wdGlvbnMudmlkZW8ua2V5RnJhbWVJbnRlcnZhbDtcbiAgICByZXR1cm4gdGhpcy5fc2lnbmFsaW5nLnNlbmRTaWduYWxpbmdNZXNzYWdlKCdzdWJzY3JpcHRpb24tY29udHJvbCcsIHtcbiAgICAgIGlkOiBzZXNzaW9uSWQsXG4gICAgICBvcGVyYXRpb246ICd1cGRhdGUnLFxuICAgICAgZGF0YToge1xuICAgICAgICB2aWRlbzoge3BhcmFtZXRlcnM6IHZpZGVvT3B0aW9uc30sXG4gICAgICB9LFxuICAgIH0pLnRoZW4oKTtcbiAgfVxuXG4gIF9vblJlbW90ZVN0cmVhbUFkZGVkKGV2ZW50KSB7XG4gICAgTG9nZ2VyLmRlYnVnKCdSZW1vdGUgc3RyZWFtIGFkZGVkLicpO1xuICAgIGZvciAoY29uc3QgW2ludGVybmFsSWQsIHN1Yl0gb2YgdGhpcy5fc3Vic2NyaWJlVHJhbnNjZWl2ZXJzKSB7XG4gICAgICBpZiAoc3ViLnRyYW5zY2VpdmVycy5maW5kKCh0KSA9PiB0LnRyYW5zY2VpdmVyID09PSBldmVudC50cmFuc2NlaXZlcikpIHtcbiAgICAgICAgaWYgKHRoaXMuX3N1YnNjcmlwdGlvbnMuaGFzKHN1Yi5pZCkpIHtcbiAgICAgICAgICBjb25zdCBzdWJzY3JpcHRpb24gPSB0aGlzLl9zdWJzY3JpcHRpb25zLmdldChzdWIuaWQpO1xuICAgICAgICAgIHN1YnNjcmlwdGlvbi5zdHJlYW0gPSBldmVudC5zdHJlYW1zWzBdO1xuICAgICAgICAgIGlmICh0aGlzLl9zdWJzY3JpYmVQcm9taXNlcy5oYXMoaW50ZXJuYWxJZCkpIHtcbiAgICAgICAgICAgIHRoaXMuX3N1YnNjcmliZVByb21pc2VzLmdldChpbnRlcm5hbElkKS5yZXNvbHZlKHN1YnNjcmlwdGlvbik7XG4gICAgICAgICAgICB0aGlzLl9zdWJzY3JpYmVQcm9taXNlcy5kZWxldGUoaW50ZXJuYWxJZCk7XG4gICAgICAgICAgfVxuICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgIHRoaXMuX3JlbW90ZU1lZGlhU3RyZWFtcy5zZXQoc3ViLmlkLCBldmVudC5zdHJlYW1zWzBdKTtcbiAgICAgICAgfVxuICAgICAgICByZXR1cm47XG4gICAgICB9XG4gICAgfVxuICAgIC8vIFRoaXMgaXMgbm90IGV4cGVjdGVkIHBhdGguIEhvd2V2ZXIsIHRoaXMgaXMgZ29pbmcgdG8gaGFwcGVuIG9uIFNhZmFyaVxuICAgIC8vIGJlY2F1c2UgaXQgZG9lcyBub3Qgc3VwcG9ydCBzZXR0aW5nIGRpcmVjdGlvbiBvZiB0cmFuc2NlaXZlci5cbiAgICBMb2dnZXIud2FybmluZygnUmVjZWl2ZWQgcmVtb3RlIHN0cmVhbSB3aXRob3V0IHN1YnNjcmlwdGlvbi4nKTtcbiAgfVxuXG4gIF9vbkxvY2FsSWNlQ2FuZGlkYXRlKGV2ZW50KSB7XG4gICAgaWYgKGV2ZW50LmNhbmRpZGF0ZSkge1xuICAgICAgaWYgKHRoaXMucGMuc2lnbmFsaW5nU3RhdGUgIT09ICdzdGFibGUnKSB7XG4gICAgICAgIHRoaXMuX3BlbmRpbmdDYW5kaWRhdGVzLnB1c2goZXZlbnQuY2FuZGlkYXRlKTtcbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIHRoaXMuX3NlbmRDYW5kaWRhdGUoZXZlbnQuY2FuZGlkYXRlKTtcbiAgICAgIH1cbiAgICB9IGVsc2Uge1xuICAgICAgTG9nZ2VyLmRlYnVnKCdFbXB0eSBjYW5kaWRhdGUuJyk7XG4gICAgfVxuICB9XG5cbiAgX2ZpcmVFbmRlZEV2ZW50T25QdWJsaWNhdGlvbk9yU3Vic2NyaXB0aW9uKCkge1xuICAgIGlmICh0aGlzLl9lbmRlZCkge1xuICAgICAgcmV0dXJuO1xuICAgIH1cbiAgICB0aGlzLl9lbmRlZCA9IHRydWU7XG4gICAgY29uc3QgZXZlbnQgPSBuZXcgT3d0RXZlbnQoJ2VuZGVkJyk7XG4gICAgZm9yIChjb25zdCBbLyogaWQgKi8sIHB1YmxpY2F0aW9uXSBvZiB0aGlzLl9wdWJsaWNhdGlvbnMpIHtcbiAgICAgIHB1YmxpY2F0aW9uLmRpc3BhdGNoRXZlbnQoZXZlbnQpO1xuICAgICAgcHVibGljYXRpb24uc3RvcCgpO1xuICAgIH1cbiAgICBmb3IgKGNvbnN0IFsvKiBpZCAqLywgc3Vic2NyaXB0aW9uXSBvZiB0aGlzLl9zdWJzY3JpcHRpb25zKSB7XG4gICAgICBzdWJzY3JpcHRpb24uZGlzcGF0Y2hFdmVudChldmVudCk7XG4gICAgICBzdWJzY3JpcHRpb24uc3RvcCgpO1xuICAgIH1cbiAgICB0aGlzLmRpc3BhdGNoRXZlbnQoZXZlbnQpO1xuICAgIHRoaXMuY2xvc2UoKTtcbiAgfVxuXG4gIF9yZWplY3RQcm9taXNlKGVycm9yKSB7XG4gICAgaWYgKCFlcnJvcikge1xuICAgICAgZXJyb3IgPSBuZXcgQ29uZmVyZW5jZUVycm9yKCdDb25uZWN0aW9uIGZhaWxlZCBvciBjbG9zZWQuJyk7XG4gICAgfVxuICAgIGlmICh0aGlzLnBjICYmIHRoaXMucGMuaWNlQ29ubmVjdGlvblN0YXRlICE9PSAnY2xvc2VkJykge1xuICAgICAgdGhpcy5wYy5jbG9zZSgpO1xuICAgIH1cblxuICAgIC8vIFJlamVjdGluZyBhbGwgY29ycmVzcG9uZGluZyBwcm9taXNlcyBpZiBwdWJsaXNoaW5nIGFuZCBzdWJzY3JpYmluZyBpcyBvbmdvaW5nLlxuICAgIGZvciAoY29uc3QgWy8qIGlkICovLCBwcm9taXNlXSBvZiB0aGlzLl9wdWJsaXNoUHJvbWlzZXMpIHtcbiAgICAgIHByb21pc2UucmVqZWN0KGVycm9yKTtcbiAgICB9XG4gICAgdGhpcy5fcHVibGlzaFByb21pc2VzLmNsZWFyKCk7XG4gICAgZm9yIChjb25zdCBbLyogaWQgKi8sIHByb21pc2VdIG9mIHRoaXMuX3N1YnNjcmliZVByb21pc2VzKSB7XG4gICAgICBwcm9taXNlLnJlamVjdChlcnJvcik7XG4gICAgfVxuICAgIHRoaXMuX3N1YnNjcmliZVByb21pc2VzLmNsZWFyKCk7XG4gIH1cblxuICBfb25JY2VDb25uZWN0aW9uU3RhdGVDaGFuZ2UoZXZlbnQpIHtcbiAgICBpZiAoIWV2ZW50IHx8ICFldmVudC5jdXJyZW50VGFyZ2V0KSB7XG4gICAgICByZXR1cm47XG4gICAgfVxuXG4gICAgTG9nZ2VyLmRlYnVnKCdJQ0UgY29ubmVjdGlvbiBzdGF0ZSBjaGFuZ2VkIHRvICcgK1xuICAgICAgICBldmVudC5jdXJyZW50VGFyZ2V0LmljZUNvbm5lY3Rpb25TdGF0ZSk7XG4gICAgaWYgKGV2ZW50LmN1cnJlbnRUYXJnZXQuaWNlQ29ubmVjdGlvblN0YXRlID09PSAnY2xvc2VkJyB8fFxuICAgICAgICBldmVudC5jdXJyZW50VGFyZ2V0LmljZUNvbm5lY3Rpb25TdGF0ZSA9PT0gJ2ZhaWxlZCcpIHtcbiAgICAgIGlmIChldmVudC5jdXJyZW50VGFyZ2V0LmljZUNvbm5lY3Rpb25TdGF0ZSA9PT0gJ2ZhaWxlZCcpIHtcbiAgICAgICAgdGhpcy5faGFuZGxlRXJyb3IoJ2Nvbm5lY3Rpb24gZmFpbGVkLicpO1xuICAgICAgfSBlbHNlIHtcbiAgICAgICAgLy8gRmlyZSBlbmRlZCBldmVudCBpZiBwdWJsaWNhdGlvbiBvciBzdWJzY3JpcHRpb24gZXhpc3RzLlxuICAgICAgICB0aGlzLl9maXJlRW5kZWRFdmVudE9uUHVibGljYXRpb25PclN1YnNjcmlwdGlvbigpO1xuICAgICAgfVxuICAgIH1cbiAgfVxuXG4gIF9vbkNvbm5lY3Rpb25TdGF0ZUNoYW5nZShldmVudCkge1xuICAgIGlmICh0aGlzLnBjLmNvbm5lY3Rpb25TdGF0ZSA9PT0gJ2Nsb3NlZCcgfHxcbiAgICAgICAgdGhpcy5wYy5jb25uZWN0aW9uU3RhdGUgPT09ICdmYWlsZWQnKSB7XG4gICAgICBpZiAodGhpcy5wYy5jb25uZWN0aW9uU3RhdGUgPT09ICdmYWlsZWQnKSB7XG4gICAgICAgIHRoaXMuX2hhbmRsZUVycm9yKCdjb25uZWN0aW9uIGZhaWxlZC4nKTtcbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIC8vIEZpcmUgZW5kZWQgZXZlbnQgaWYgcHVibGljYXRpb24gb3Igc3Vic2NyaXB0aW9uIGV4aXN0cy5cbiAgICAgICAgdGhpcy5fZmlyZUVuZGVkRXZlbnRPblB1YmxpY2F0aW9uT3JTdWJzY3JpcHRpb24oKTtcbiAgICAgIH1cbiAgICB9XG4gIH1cblxuICBfc2VuZENhbmRpZGF0ZShjYW5kaWRhdGUpIHtcbiAgICB0aGlzLl9zaWduYWxpbmcuc2VuZFNpZ25hbGluZ01lc3NhZ2UoJ3NvYWMnLCB7XG4gICAgICBpZDogdGhpcy5faWQsXG4gICAgICBzaWduYWxpbmc6IHtcbiAgICAgICAgdHlwZTogJ2NhbmRpZGF0ZScsXG4gICAgICAgIGNhbmRpZGF0ZToge1xuICAgICAgICAgIGNhbmRpZGF0ZTogJ2E9JyArIGNhbmRpZGF0ZS5jYW5kaWRhdGUsXG4gICAgICAgICAgc2RwTWlkOiBjYW5kaWRhdGUuc2RwTWlkLFxuICAgICAgICAgIHNkcE1MaW5lSW5kZXg6IGNhbmRpZGF0ZS5zZHBNTGluZUluZGV4LFxuICAgICAgICB9LFxuICAgICAgfSxcbiAgICB9KTtcbiAgfVxuXG4gIF9jcmVhdGVQZWVyQ29ubmVjdGlvbigpIHtcbiAgICBpZiAodGhpcy5wYykge1xuICAgICAgcmV0dXJuO1xuICAgIH1cblxuICAgIGNvbnN0IHBjQ29uZmlndXJhdGlvbiA9IHRoaXMuX2NvbmZpZy5ydGNDb25maWd1cmF0aW9uIHx8IHt9O1xuICAgIGlmIChVdGlscy5pc0Nocm9tZSgpKSB7XG4gICAgICBwY0NvbmZpZ3VyYXRpb24uYnVuZGxlUG9saWN5ID0gJ21heC1idW5kbGUnO1xuICAgIH1cbiAgICB0aGlzLnBjID0gbmV3IFJUQ1BlZXJDb25uZWN0aW9uKHBjQ29uZmlndXJhdGlvbik7XG4gICAgdGhpcy5wYy5vbmljZWNhbmRpZGF0ZSA9IChldmVudCkgPT4ge1xuICAgICAgdGhpcy5fb25Mb2NhbEljZUNhbmRpZGF0ZS5hcHBseSh0aGlzLCBbZXZlbnRdKTtcbiAgICB9O1xuICAgIHRoaXMucGMub250cmFjayA9IChldmVudCkgPT4ge1xuICAgICAgdGhpcy5fb25SZW1vdGVTdHJlYW1BZGRlZC5hcHBseSh0aGlzLCBbZXZlbnRdKTtcbiAgICB9O1xuICAgIHRoaXMucGMub25pY2Vjb25uZWN0aW9uc3RhdGVjaGFuZ2UgPSAoZXZlbnQpID0+IHtcbiAgICAgIHRoaXMuX29uSWNlQ29ubmVjdGlvblN0YXRlQ2hhbmdlLmFwcGx5KHRoaXMsIFtldmVudF0pO1xuICAgIH07XG4gICAgdGhpcy5wYy5vbmNvbm5lY3Rpb25zdGF0ZWNoYW5nZSA9IChldmVudCkgPT4ge1xuICAgICAgdGhpcy5fb25Db25uZWN0aW9uU3RhdGVDaGFuZ2UuYXBwbHkodGhpcywgW2V2ZW50XSk7XG4gICAgfTtcbiAgfVxuXG4gIF9nZXRTdGF0cygpIHtcbiAgICBpZiAodGhpcy5wYykge1xuICAgICAgcmV0dXJuIHRoaXMucGMuZ2V0U3RhdHMoKTtcbiAgICB9IGVsc2Uge1xuICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KG5ldyBDb25mZXJlbmNlRXJyb3IoXG4gICAgICAgICAgJ1BlZXJDb25uZWN0aW9uIGlzIG5vdCBhdmFpbGFibGUuJykpO1xuICAgIH1cbiAgfVxuXG4gIF9yZWFkeUhhbmRsZXIoc2Vzc2lvbklkKSB7XG4gICAgY29uc3QgaW50ZXJuYWxJZCA9IHRoaXMuX3JldmVyc2VJZE1hcC5nZXQoc2Vzc2lvbklkKTtcbiAgICBpZiAodGhpcy5fc3Vic2NyaWJlUHJvbWlzZXMuaGFzKGludGVybmFsSWQpKSB7XG4gICAgICBjb25zdCBtZWRpYVN0cmVhbSA9IHRoaXMuX3JlbW90ZU1lZGlhU3RyZWFtcy5nZXQoc2Vzc2lvbklkKTtcbiAgICAgIGNvbnN0IHRyYW5zcG9ydFNldHRpbmdzID1cbiAgICAgICAgICBuZXcgVHJhbnNwb3J0U2V0dGluZ3MoVHJhbnNwb3J0VHlwZS5XRUJSVEMsIHRoaXMuX2lkKTtcbiAgICAgIHRyYW5zcG9ydFNldHRpbmdzLnJ0cFRyYW5zY2VpdmVycyA9XG4gICAgICAgICAgdGhpcy5fc3Vic2NyaWJlVHJhbnNjZWl2ZXJzLmdldChpbnRlcm5hbElkKS50cmFuc2NlaXZlcnM7XG4gICAgICBjb25zdCBzdWJzY3JpcHRpb24gPSBuZXcgU3Vic2NyaXB0aW9uKFxuICAgICAgICAgIHNlc3Npb25JZCwgbWVkaWFTdHJlYW0sIHRyYW5zcG9ydFNldHRpbmdzLFxuICAgICAgICAgICgpID0+IHtcbiAgICAgICAgICAgIHRoaXMuX3Vuc3Vic2NyaWJlKGludGVybmFsSWQpO1xuICAgICAgICAgIH0sXG4gICAgICAgICAgKCkgPT4gdGhpcy5fZ2V0U3RhdHMoKSxcbiAgICAgICAgICAodHJhY2tLaW5kKSA9PiB0aGlzLl9tdXRlT3JVbm11dGUoc2Vzc2lvbklkLCB0cnVlLCBmYWxzZSwgdHJhY2tLaW5kKSxcbiAgICAgICAgICAodHJhY2tLaW5kKSA9PiB0aGlzLl9tdXRlT3JVbm11dGUoc2Vzc2lvbklkLCBmYWxzZSwgZmFsc2UsIHRyYWNrS2luZCksXG4gICAgICAgICAgKG9wdGlvbnMpID0+IHRoaXMuX2FwcGx5T3B0aW9ucyhzZXNzaW9uSWQsIG9wdGlvbnMpKTtcbiAgICAgIHRoaXMuX3N1YnNjcmlwdGlvbnMuc2V0KHNlc3Npb25JZCwgc3Vic2NyaXB0aW9uKTtcbiAgICAgIC8vIFJlc29sdmUgc3Vic2NyaXB0aW9uIGlmIG1lZGlhU3RyZWFtIGlzIHJlYWR5LlxuICAgICAgaWYgKHRoaXMuX3N1YnNjcmlwdGlvbnMuZ2V0KHNlc3Npb25JZCkuc3RyZWFtKSB7XG4gICAgICAgIHRoaXMuX3N1YnNjcmliZVByb21pc2VzLmdldChpbnRlcm5hbElkKS5yZXNvbHZlKHN1YnNjcmlwdGlvbik7XG4gICAgICAgIHRoaXMuX3N1YnNjcmliZVByb21pc2VzLmRlbGV0ZShpbnRlcm5hbElkKTtcbiAgICAgIH1cbiAgICB9IGVsc2UgaWYgKHRoaXMuX3B1Ymxpc2hQcm9taXNlcy5oYXMoaW50ZXJuYWxJZCkpIHtcbiAgICAgIGNvbnN0IHRyYW5zcG9ydFNldHRpbmdzID1cbiAgICAgICAgICBuZXcgVHJhbnNwb3J0U2V0dGluZ3MoVHJhbnNwb3J0VHlwZS5XRUJSVEMsIHRoaXMuX2lkKTtcbiAgICAgIHRyYW5zcG9ydFNldHRpbmdzLnRyYW5zY2VpdmVycyA9XG4gICAgICAgICAgdGhpcy5fcHVibGlzaFRyYW5zY2VpdmVycy5nZXQoaW50ZXJuYWxJZCkudHJhbnNjZWl2ZXJzO1xuICAgICAgY29uc3QgcHVibGljYXRpb24gPSBuZXcgUHVibGljYXRpb24oXG4gICAgICAgICAgc2Vzc2lvbklkLFxuICAgICAgICAgIHRyYW5zcG9ydFNldHRpbmdzLFxuICAgICAgICAgICgpID0+IHtcbiAgICAgICAgICAgIHRoaXMuX3VucHVibGlzaChpbnRlcm5hbElkKTtcbiAgICAgICAgICAgIHJldHVybiBQcm9taXNlLnJlc29sdmUoKTtcbiAgICAgICAgICB9LFxuICAgICAgICAgICgpID0+IHRoaXMuX2dldFN0YXRzKCksXG4gICAgICAgICAgKHRyYWNrS2luZCkgPT4gdGhpcy5fbXV0ZU9yVW5tdXRlKHNlc3Npb25JZCwgdHJ1ZSwgdHJ1ZSwgdHJhY2tLaW5kKSxcbiAgICAgICAgICAodHJhY2tLaW5kKSA9PiB0aGlzLl9tdXRlT3JVbm11dGUoc2Vzc2lvbklkLCBmYWxzZSwgdHJ1ZSwgdHJhY2tLaW5kKSk7XG4gICAgICB0aGlzLl9wdWJsaWNhdGlvbnMuc2V0KHNlc3Npb25JZCwgcHVibGljYXRpb24pO1xuICAgICAgdGhpcy5fcHVibGlzaFByb21pc2VzLmdldChpbnRlcm5hbElkKS5yZXNvbHZlKHB1YmxpY2F0aW9uKTtcbiAgICAgIC8vIERvIG5vdCBmaXJlIHB1YmxpY2F0aW9uJ3MgZW5kZWQgZXZlbnQgd2hlbiBhc3NvY2lhdGVkIHN0cmVhbSBpcyBlbmRlZC5cbiAgICAgIC8vIEl0IG1heSBzdGlsbCBzZW5kaW5nIHNpbGVuY2Ugb3IgYmxhY2sgZnJhbWVzLlxuICAgICAgLy8gUmVmZXIgdG8gaHR0cHM6Ly93M2MuZ2l0aHViLmlvL3dlYnJ0Yy1wYy8jcnRjcnRwc2VuZGVyLWludGVyZmFjZS5cbiAgICB9IGVsc2UgaWYgKCFzZXNzaW9uSWQpIHtcbiAgICAgIC8vIENoYW5uZWwgcmVhZHlcbiAgICB9XG4gIH1cblxuICBfc2RwSGFuZGxlcihzZHApIHtcbiAgICBpZiAoc2RwLnR5cGUgPT09ICdhbnN3ZXInKSB7XG4gICAgICB0aGlzLnBjLnNldFJlbW90ZURlc2NyaXB0aW9uKHNkcCkudGhlbigoKSA9PiB7XG4gICAgICAgIGlmICh0aGlzLl9wZW5kaW5nQ2FuZGlkYXRlcy5sZW5ndGggPiAwKSB7XG4gICAgICAgICAgZm9yIChjb25zdCBjYW5kaWRhdGUgb2YgdGhpcy5fcGVuZGluZ0NhbmRpZGF0ZXMpIHtcbiAgICAgICAgICAgIHRoaXMuX3NlbmRDYW5kaWRhdGUoY2FuZGlkYXRlKTtcbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgIH0sIChlcnJvcikgPT4ge1xuICAgICAgICBMb2dnZXIuZXJyb3IoJ1NldCByZW1vdGUgZGVzY3JpcHRpb24gZmFpbGVkOiAnICsgZXJyb3IpO1xuICAgICAgICB0aGlzLl9yZWplY3RQcm9taXNlKGVycm9yKTtcbiAgICAgICAgdGhpcy5fZmlyZUVuZGVkRXZlbnRPblB1YmxpY2F0aW9uT3JTdWJzY3JpcHRpb24oKTtcbiAgICAgIH0pLnRoZW4oKCkgPT4ge1xuICAgICAgICBpZiAoIXRoaXMuX25leHRTZHBQcm9taXNlKCkpIHtcbiAgICAgICAgICBMb2dnZXIud2FybmluZygnVW5leHBlY3RlZCBTRFAgcHJvbWlzZSBzdGF0ZScpO1xuICAgICAgICB9XG4gICAgICB9KTtcbiAgICB9XG4gIH1cblxuICBfZXJyb3JIYW5kbGVyKHNlc3Npb25JZCwgZXJyb3JNZXNzYWdlKSB7XG4gICAgaWYgKCFzZXNzaW9uSWQpIHtcbiAgICAgIC8vIFRyYW5zcG9ydCBlcnJvclxuICAgICAgcmV0dXJuIHRoaXMuX2hhbmRsZUVycm9yKGVycm9yTWVzc2FnZSk7XG4gICAgfVxuXG4gICAgLy8gRmlyZSBlcnJvciBldmVudCBvbiBwdWJsaWNhdGlvbiBvciBzdWJzY3JpcHRpb25cbiAgICBjb25zdCBlcnJvckV2ZW50ID0gbmV3IEVycm9yRXZlbnQoJ2Vycm9yJywge1xuICAgICAgZXJyb3I6IG5ldyBDb25mZXJlbmNlRXJyb3IoZXJyb3JNZXNzYWdlKSxcbiAgICB9KTtcbiAgICBpZiAodGhpcy5fcHVibGljYXRpb25zLmhhcyhzZXNzaW9uSWQpKSB7XG4gICAgICB0aGlzLl9wdWJsaWNhdGlvbnMuZ2V0KHNlc3Npb25JZCkuZGlzcGF0Y2hFdmVudChlcnJvckV2ZW50KTtcbiAgICB9XG4gICAgaWYgKHRoaXMuX3N1YnNjcmlwdGlvbnMuaGFzKHNlc3Npb25JZCkpIHtcbiAgICAgIHRoaXMuX3N1YnNjcmlwdGlvbnMuZ2V0KHNlc3Npb25JZCkuZGlzcGF0Y2hFdmVudChlcnJvckV2ZW50KTtcbiAgICB9XG4gICAgLy8gU3RvcCBwdWJsaWNhdGlvbiBvciBzdWJzY3JpcHRpb25cbiAgICBjb25zdCBpbnRlcm5hbElkID0gdGhpcy5fcmV2ZXJzZUlkTWFwLmdldChzZXNzaW9uSWQpO1xuICAgIGlmICh0aGlzLl9wdWJsaXNoVHJhbnNjZWl2ZXJzLmhhcyhpbnRlcm5hbElkKSkge1xuICAgICAgdGhpcy5fdW5wdWJsaXNoKGludGVybmFsSWQpO1xuICAgIH1cbiAgICBpZiAodGhpcy5fc3Vic2NyaWJlVHJhbnNjZWl2ZXJzLmhhcyhpbnRlcm5hbElkKSkge1xuICAgICAgdGhpcy5fdW5zdWJzY3JpYmUoaW50ZXJuYWxJZCk7XG4gICAgfVxuICB9XG5cbiAgX2hhbmRsZUVycm9yKGVycm9yTWVzc2FnZSkge1xuICAgIGNvbnN0IGVycm9yID0gbmV3IENvbmZlcmVuY2VFcnJvcihlcnJvck1lc3NhZ2UpO1xuICAgIGlmICh0aGlzLl9lbmRlZCkge1xuICAgICAgcmV0dXJuO1xuICAgIH1cbiAgICBjb25zdCBlcnJvckV2ZW50ID0gbmV3IEVycm9yRXZlbnQoJ2Vycm9yJywge1xuICAgICAgZXJyb3I6IGVycm9yLFxuICAgIH0pO1xuICAgIGZvciAoY29uc3QgWy8qIGlkICovLCBwdWJsaWNhdGlvbl0gb2YgdGhpcy5fcHVibGljYXRpb25zKSB7XG4gICAgICBwdWJsaWNhdGlvbi5kaXNwYXRjaEV2ZW50KGVycm9yRXZlbnQpO1xuICAgIH1cbiAgICBmb3IgKGNvbnN0IFsvKiBpZCAqLywgc3Vic2NyaXB0aW9uXSBvZiB0aGlzLl9zdWJzY3JpcHRpb25zKSB7XG4gICAgICBzdWJzY3JpcHRpb24uZGlzcGF0Y2hFdmVudChlcnJvckV2ZW50KTtcbiAgICB9XG4gICAgLy8gRmlyZSBlbmRlZCBldmVudCB3aGVuIGVycm9yIG9jY3VyZWRcbiAgICB0aGlzLl9maXJlRW5kZWRFdmVudE9uUHVibGljYXRpb25PclN1YnNjcmlwdGlvbigpO1xuICB9XG5cbiAgX3NldENvZGVjT3JkZXIoc2RwLCBvcHRpb25zLCBtaWQpIHtcbiAgICBpZiAob3B0aW9ucy5hdWRpbykge1xuICAgICAgaWYgKG9wdGlvbnMuYXVkaW8uY29kZWNzKSB7XG4gICAgICAgIGNvbnN0IGF1ZGlvQ29kZWNOYW1lcyA9IEFycmF5LmZyb20ob3B0aW9ucy5hdWRpby5jb2RlY3MsIChjb2RlYykgPT5cbiAgICAgICAgICBjb2RlYy5uYW1lKTtcbiAgICAgICAgc2RwID0gU2RwVXRpbHMucmVvcmRlckNvZGVjcyhzZHAsICdhdWRpbycsIGF1ZGlvQ29kZWNOYW1lcywgbWlkKTtcbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIGNvbnN0IGF1ZGlvQ29kZWNOYW1lcyA9IEFycmF5LmZyb20ob3B0aW9ucy5hdWRpbyxcbiAgICAgICAgICAgIChlbmNvZGluZ1BhcmFtZXRlcnMpID0+IGVuY29kaW5nUGFyYW1ldGVycy5jb2RlYy5uYW1lKTtcbiAgICAgICAgc2RwID0gU2RwVXRpbHMucmVvcmRlckNvZGVjcyhzZHAsICdhdWRpbycsIGF1ZGlvQ29kZWNOYW1lcywgbWlkKTtcbiAgICAgIH1cbiAgICB9XG4gICAgaWYgKG9wdGlvbnMudmlkZW8pIHtcbiAgICAgIGlmIChvcHRpb25zLnZpZGVvLmNvZGVjcykge1xuICAgICAgICBjb25zdCB2aWRlb0NvZGVjTmFtZXMgPSBBcnJheS5mcm9tKG9wdGlvbnMudmlkZW8uY29kZWNzLCAoY29kZWMpID0+XG4gICAgICAgICAgY29kZWMubmFtZSk7XG4gICAgICAgIHNkcCA9IFNkcFV0aWxzLnJlb3JkZXJDb2RlY3Moc2RwLCAndmlkZW8nLCB2aWRlb0NvZGVjTmFtZXMsIG1pZCk7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICBjb25zdCB2aWRlb0NvZGVjTmFtZXMgPSBBcnJheS5mcm9tKG9wdGlvbnMudmlkZW8sXG4gICAgICAgICAgICAoZW5jb2RpbmdQYXJhbWV0ZXJzKSA9PiBlbmNvZGluZ1BhcmFtZXRlcnMuY29kZWMubmFtZSk7XG4gICAgICAgIHNkcCA9IFNkcFV0aWxzLnJlb3JkZXJDb2RlY3Moc2RwLCAndmlkZW8nLCB2aWRlb0NvZGVjTmFtZXMsIG1pZCk7XG4gICAgICB9XG4gICAgfVxuICAgIHJldHVybiBzZHA7XG4gIH1cblxuICBfc2V0TWF4Qml0cmF0ZShzZHAsIG9wdGlvbnMsIG1pZCkge1xuICAgIGlmICh0eXBlb2Ygb3B0aW9ucy5hdWRpbyA9PT0gJ29iamVjdCcpIHtcbiAgICAgIHNkcCA9IFNkcFV0aWxzLnNldE1heEJpdHJhdGUoc2RwLCBvcHRpb25zLmF1ZGlvLCBtaWQpO1xuICAgIH1cbiAgICBpZiAodHlwZW9mIG9wdGlvbnMudmlkZW8gPT09ICdvYmplY3QnKSB7XG4gICAgICBzZHAgPSBTZHBVdGlscy5zZXRNYXhCaXRyYXRlKHNkcCwgb3B0aW9ucy52aWRlbywgbWlkKTtcbiAgICB9XG4gICAgcmV0dXJuIHNkcDtcbiAgfVxuXG4gIF9zZXRSdHBTZW5kZXJPcHRpb25zKHNkcCwgb3B0aW9ucywgbWlkKSB7XG4gICAgLy8gU0RQIG11Z2xpbmcgaXMgZGVwcmVjYXRlZCwgbW92aW5nIHRvIGBzZXRQYXJhbWV0ZXJzYC5cbiAgICBpZiAodGhpcy5faXNSdHBFbmNvZGluZ1BhcmFtZXRlcnMob3B0aW9ucy5hdWRpbykgfHxcbiAgICAgICAgdGhpcy5faXNSdHBFbmNvZGluZ1BhcmFtZXRlcnMob3B0aW9ucy52aWRlbykpIHtcbiAgICAgIHJldHVybiBzZHA7XG4gICAgfVxuICAgIHNkcCA9IHRoaXMuX3NldE1heEJpdHJhdGUoc2RwLCBvcHRpb25zLCBtaWQpO1xuICAgIHJldHVybiBzZHA7XG4gIH1cblxuICBfc2V0UnRwUmVjZWl2ZXJPcHRpb25zKHNkcCwgb3B0aW9ucywgbWlkKSB7XG4gICAgLy8gQWRkIGxlZ2FjeSBzaW11bGNhc3QgaW4gU0RQIGZvciBzYWZhcmkuXG4gICAgaWYgKHRoaXMuX2lzUnRwRW5jb2RpbmdQYXJhbWV0ZXJzKG9wdGlvbnMudmlkZW8pICYmIFV0aWxzLmlzU2FmYXJpKCkpIHtcbiAgICAgIGlmIChvcHRpb25zLnZpZGVvLmxlbmd0aCA+IDEpIHtcbiAgICAgICAgc2RwID0gU2RwVXRpbHMuYWRkTGVnYWN5U2ltdWxjYXN0KFxuICAgICAgICAgICAgc2RwLCAndmlkZW8nLCBvcHRpb25zLnZpZGVvLmxlbmd0aCwgbWlkKTtcbiAgICAgIH1cbiAgICB9XG5cbiAgICAvLyBfdmlkZW9Db2RlY3MgaXMgYSB3b3JrYXJvdW5kIGZvciBzZXR0aW5nIHZpZGVvIGNvZGVjcy4gSXQgd2lsbCBiZSBtb3ZlZCB0byBSVENSdHBTZW5kUGFyYW1ldGVycy5cbiAgICBpZiAodGhpcy5faXNSdHBFbmNvZGluZ1BhcmFtZXRlcnMob3B0aW9ucy52aWRlbykgJiYgdGhpcy5fdmlkZW9Db2RlY3MpIHtcbiAgICAgIHNkcCA9IFNkcFV0aWxzLnJlb3JkZXJDb2RlY3Moc2RwLCAndmlkZW8nLCB0aGlzLl92aWRlb0NvZGVjcywgbWlkKTtcbiAgICAgIHJldHVybiBzZHA7XG4gICAgfVxuICAgIGlmICh0aGlzLl9pc1J0cEVuY29kaW5nUGFyYW1ldGVycyhvcHRpb25zLmF1ZGlvKSB8fFxuICAgICAgICB0aGlzLl9pc1J0cEVuY29kaW5nUGFyYW1ldGVycyhvcHRpb25zLnZpZGVvKSkge1xuICAgICAgcmV0dXJuIHNkcDtcbiAgICB9XG4gICAgc2RwID0gdGhpcy5fc2V0Q29kZWNPcmRlcihzZHAsIG9wdGlvbnMsIG1pZCk7XG4gICAgcmV0dXJuIHNkcDtcbiAgfVxuXG4gIC8vIEhhbmRsZSBzdHJlYW0gZXZlbnQgc2VudCBmcm9tIE1DVS4gU29tZSBzdHJlYW0gdXBkYXRlIGV2ZW50cyBzZW50IGZyb21cbiAgLy8gc2VydmVyLCBtb3JlIHNwZWNpZmljYWxseSBhdWRpby5zdGF0dXMgYW5kIHZpZGVvLnN0YXR1cyBldmVudHMgc2hvdWxkIGJlXG4gIC8vIHB1YmxpY2F0aW9uIGV2ZW50IG9yIHN1YnNjcmlwdGlvbiBldmVudHMuIFRoZXkgZG9uJ3QgY2hhbmdlIE1lZGlhU3RyZWFtJ3NcbiAgLy8gc3RhdHVzLiBTZWVcbiAgLy8gaHR0cHM6Ly9naXRodWIuY29tL29wZW4td2VicnRjLXRvb2xraXQvb3d0LXNlcnZlci9ibG9iL21hc3Rlci9kb2MvQ2xpZW50LVBvcnRhbCUyMFByb3RvY29sLm1kIzMzOS1wYXJ0aWNpcGFudC1pcy1ub3RpZmllZC1vbi1zdHJlYW1zLXVwZGF0ZS1pbi1yb29tXG4gIC8vIGZvciBtb3JlIGluZm9ybWF0aW9uLlxuICBfb25TdHJlYW1FdmVudChtZXNzYWdlKSB7XG4gICAgY29uc3QgZXZlbnRUYXJnZXRzID0gW107XG4gICAgaWYgKHRoaXMuX3B1YmxpY2F0aW9ucy5oYXMobWVzc2FnZS5pZCkpIHtcbiAgICAgIGV2ZW50VGFyZ2V0cy5wdXNoKHRoaXMuX3B1YmxpY2F0aW9ucy5nZXQobWVzc2FnZS5pZCkpO1xuICAgIH1cbiAgICBmb3IgKGNvbnN0IHN1YnNjcmlwdGlvbiBvZiB0aGlzLl9zdWJzY3JpcHRpb25zKSB7XG4gICAgICBpZiAobWVzc2FnZS5pZCA9PT0gc3Vic2NyaXB0aW9uLl9hdWRpb1RyYWNrSWQgfHxcbiAgICAgICAgICBtZXNzYWdlLmlkID09PSBzdWJzY3JpcHRpb24uX3ZpZGVvVHJhY2tJZCkge1xuICAgICAgICBldmVudFRhcmdldHMucHVzaChzdWJzY3JpcHRpb24pO1xuICAgICAgfVxuICAgIH1cbiAgICBpZiAoIWV2ZW50VGFyZ2V0cy5sZW5ndGgpIHtcbiAgICAgIHJldHVybjtcbiAgICB9XG4gICAgbGV0IHRyYWNrS2luZDtcbiAgICBpZiAobWVzc2FnZS5kYXRhLmZpZWxkID09PSAnYXVkaW8uc3RhdHVzJykge1xuICAgICAgdHJhY2tLaW5kID0gVHJhY2tLaW5kLkFVRElPO1xuICAgIH0gZWxzZSBpZiAobWVzc2FnZS5kYXRhLmZpZWxkID09PSAndmlkZW8uc3RhdHVzJykge1xuICAgICAgdHJhY2tLaW5kID0gVHJhY2tLaW5kLlZJREVPO1xuICAgIH0gZWxzZSB7XG4gICAgICBMb2dnZXIud2FybmluZygnSW52YWxpZCBkYXRhIGZpZWxkIGZvciBzdHJlYW0gdXBkYXRlIGluZm8uJyk7XG4gICAgfVxuICAgIGlmIChtZXNzYWdlLmRhdGEudmFsdWUgPT09ICdhY3RpdmUnKSB7XG4gICAgICBldmVudFRhcmdldHMuZm9yRWFjaCgodGFyZ2V0KSA9PlxuICAgICAgICB0YXJnZXQuZGlzcGF0Y2hFdmVudChuZXcgTXV0ZUV2ZW50KCd1bm11dGUnLCB7a2luZDogdHJhY2tLaW5kfSkpKTtcbiAgICB9IGVsc2UgaWYgKG1lc3NhZ2UuZGF0YS52YWx1ZSA9PT0gJ2luYWN0aXZlJykge1xuICAgICAgZXZlbnRUYXJnZXRzLmZvckVhY2goKHRhcmdldCkgPT5cbiAgICAgICAgdGFyZ2V0LmRpc3BhdGNoRXZlbnQobmV3IE11dGVFdmVudCgnbXV0ZScsIHtraW5kOiB0cmFja0tpbmR9KSkpO1xuICAgIH0gZWxzZSB7XG4gICAgICBMb2dnZXIud2FybmluZygnSW52YWxpZCBkYXRhIHZhbHVlIGZvciBzdHJlYW0gdXBkYXRlIGluZm8uJyk7XG4gICAgfVxuICB9XG5cbiAgX2lzUnRwRW5jb2RpbmdQYXJhbWV0ZXJzKG9iaikge1xuICAgIGlmICghQXJyYXkuaXNBcnJheShvYmopKSB7XG4gICAgICByZXR1cm4gZmFsc2U7XG4gICAgfVxuICAgIC8vIE9ubHkgY2hlY2sgdGhlIGZpcnN0IG9uZS5cbiAgICBjb25zdCBwYXJhbSA9IG9ialswXTtcbiAgICByZXR1cm4gISEoXG4gICAgICBwYXJhbS5jb2RlY1BheWxvYWRUeXBlIHx8IHBhcmFtLmR0eCB8fCBwYXJhbS5hY3RpdmUgfHwgcGFyYW0ucHRpbWUgfHxcbiAgICAgIHBhcmFtLm1heEZyYW1lcmF0ZSB8fCBwYXJhbS5zY2FsZVJlc29sdXRpb25Eb3duQnkgfHwgcGFyYW0ucmlkIHx8XG4gICAgICBwYXJhbS5zY2FsYWJpbGl0eU1vZGUpO1xuICB9XG5cbiAgX2lzT3d0RW5jb2RpbmdQYXJhbWV0ZXJzKG9iaikge1xuICAgIGlmICghQXJyYXkuaXNBcnJheShvYmopKSB7XG4gICAgICByZXR1cm4gZmFsc2U7XG4gICAgfVxuICAgIC8vIE9ubHkgY2hlY2sgdGhlIGZpcnN0IG9uZS5cbiAgICBjb25zdCBwYXJhbSA9IG9ialswXTtcbiAgICByZXR1cm4gISFwYXJhbS5jb2RlYztcbiAgfVxufVxuIiwiLy8gQ29weXJpZ2h0IChDKSA8MjAxOD4gSW50ZWwgQ29ycG9yYXRpb25cbi8vXG4vLyBTUERYLUxpY2Vuc2UtSWRlbnRpZmllcjogQXBhY2hlLTIuMFxuXG4vKiBnbG9iYWwgTWFwLCBQcm9taXNlICovXG5cbid1c2Ugc3RyaWN0JztcblxuaW1wb3J0ICogYXMgRXZlbnRNb2R1bGUgZnJvbSAnLi4vYmFzZS9ldmVudC5qcyc7XG5pbXBvcnQge1Npb1NpZ25hbGluZyBhcyBTaWduYWxpbmd9IGZyb20gJy4vc2lnbmFsaW5nLmpzJztcbmltcG9ydCBMb2dnZXIgZnJvbSAnLi4vYmFzZS9sb2dnZXIuanMnO1xuaW1wb3J0IHtCYXNlNjR9IGZyb20gJy4uL2Jhc2UvYmFzZTY0LmpzJztcbmltcG9ydCB7Q29uZmVyZW5jZUVycm9yfSBmcm9tICcuL2Vycm9yLmpzJztcbmltcG9ydCAqIGFzIFV0aWxzIGZyb20gJy4uL2Jhc2UvdXRpbHMuanMnO1xuaW1wb3J0ICogYXMgU3RyZWFtTW9kdWxlIGZyb20gJy4uL2Jhc2Uvc3RyZWFtLmpzJztcbmltcG9ydCB7UGFydGljaXBhbnR9IGZyb20gJy4vcGFydGljaXBhbnQuanMnO1xuaW1wb3J0IHtDb25mZXJlbmNlSW5mb30gZnJvbSAnLi9pbmZvLmpzJztcbmltcG9ydCB7Q29uZmVyZW5jZVBlZXJDb25uZWN0aW9uQ2hhbm5lbH0gZnJvbSAnLi9jaGFubmVsLmpzJztcbmltcG9ydCB7UXVpY0Nvbm5lY3Rpb259IGZyb20gJy4vcXVpY2Nvbm5lY3Rpb24uanMnO1xuaW1wb3J0IHtSZW1vdGVNaXhlZFN0cmVhbSwgQWN0aXZlQXVkaW9JbnB1dENoYW5nZUV2ZW50LCBMYXlvdXRDaGFuZ2VFdmVudH1cbiAgZnJvbSAnLi9taXhlZHN0cmVhbS5qcyc7XG5pbXBvcnQgKiBhcyBTdHJlYW1VdGlsc01vZHVsZSBmcm9tICcuL3N0cmVhbXV0aWxzLmpzJztcblxuY29uc3QgU2lnbmFsaW5nU3RhdGUgPSB7XG4gIFJFQURZOiAxLFxuICBDT05ORUNUSU5HOiAyLFxuICBDT05ORUNURUQ6IDMsXG59O1xuXG5jb25zdCBwcm90b2NvbFZlcnNpb24gPSAnMS4yJztcblxuLyogZXNsaW50LWRpc2FibGUgdmFsaWQtanNkb2MgKi9cbi8qKlxuICogQGNsYXNzIFBhcnRpY2lwYW50RXZlbnRcbiAqIEBjbGFzc0Rlc2MgQ2xhc3MgUGFydGljaXBhbnRFdmVudCByZXByZXNlbnRzIGEgcGFydGljaXBhbnQgZXZlbnQuXG4gICBAZXh0ZW5kcyBPd3QuQmFzZS5Pd3RFdmVudFxuICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKi9cbmNvbnN0IFBhcnRpY2lwYW50RXZlbnQgPSBmdW5jdGlvbih0eXBlLCBpbml0KSB7XG4gIGNvbnN0IHRoYXQgPSBuZXcgRXZlbnRNb2R1bGUuT3d0RXZlbnQodHlwZSwgaW5pdCk7XG4gIC8qKlxuICAgKiBAbWVtYmVyIHtPd3QuQ29uZmVyZW5jZS5QYXJ0aWNpcGFudH0gcGFydGljaXBhbnRcbiAgICogQGluc3RhbmNlXG4gICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5QYXJ0aWNpcGFudEV2ZW50XG4gICAqL1xuICB0aGF0LnBhcnRpY2lwYW50ID0gaW5pdC5wYXJ0aWNpcGFudDtcbiAgcmV0dXJuIHRoYXQ7XG59O1xuLyogZXNsaW50LWVuYWJsZSB2YWxpZC1qc2RvYyAqL1xuXG4vKipcbiAqIEBjbGFzcyBDb25mZXJlbmNlQ2xpZW50Q29uZmlndXJhdGlvblxuICogQGNsYXNzRGVzYyBDb25maWd1cmF0aW9uIGZvciBDb25mZXJlbmNlQ2xpZW50LlxuICogQG1lbWJlck9mIE93dC5Db25mZXJlbmNlXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKi9cbmNsYXNzIENvbmZlcmVuY2VDbGllbnRDb25maWd1cmF0aW9uIHsgLy8gZXNsaW50LWRpc2FibGUtbGluZSBuby11bnVzZWQtdmFyc1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3RvcigpIHtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/UlRDQ29uZmlndXJhdGlvbn0gcnRjQ29uZmlndXJhdGlvblxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5Db25mZXJlbmNlQ2xpZW50Q29uZmlndXJhdGlvblxuICAgICAqIEBkZXNjIEl0IHdpbGwgYmUgdXNlZCBmb3IgY3JlYXRpbmcgUGVlckNvbm5lY3Rpb24uXG4gICAgICogQHNlZSB7QGxpbmsgaHR0cHM6Ly93d3cudzMub3JnL1RSL3dlYnJ0Yy8jcnRjY29uZmlndXJhdGlvbi1kaWN0aW9uYXJ5fFJUQ0NvbmZpZ3VyYXRpb24gRGljdGlvbmFyeSBvZiBXZWJSVEMgMS4wfS5cbiAgICAgKiBAZXhhbXBsZVxuICAgICAqIC8vIEZvbGxvd2luZyBvYmplY3QgY2FuIGJlIHNldCB0byBjb25mZXJlbmNlQ2xpZW50Q29uZmlndXJhdGlvbi5ydGNDb25maWd1cmF0aW9uLlxuICAgICAqIHtcbiAgICAgKiAgIGljZVNlcnZlcnM6IFt7XG4gICAgICogICAgICB1cmxzOiBcInN0dW46ZXhhbXBsZS5jb206MzQ3OFwiXG4gICAgICogICB9LCB7XG4gICAgICogICAgIHVybHM6IFtcbiAgICAgKiAgICAgICBcInR1cm46ZXhhbXBsZS5jb206MzQ3OD90cmFuc3BvcnQ9dWRwXCIsXG4gICAgICogICAgICAgXCJ0dXJuOmV4YW1wbGUuY29tOjM0Nzg/dHJhbnNwb3J0PXRjcFwiXG4gICAgICogICAgIF0sXG4gICAgICogICAgICBjcmVkZW50aWFsOiBcInBhc3N3b3JkXCIsXG4gICAgICogICAgICB1c2VybmFtZTogXCJ1c2VybmFtZVwiXG4gICAgICogICB9XG4gICAgICogfVxuICAgICAqL1xuICAgIHRoaXMucnRjQ29uZmlndXJhdGlvbiA9IHVuZGVmaW5lZDtcblxuICAgIC8qKlxuICAgICAqIEBtZW1iZXIgez9XZWJUcmFuc3BvcnRPcHRpb25zfSB3ZWJUcmFuc3BvcnRDb25maWd1cmF0aW9uXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLkNvbmZlcmVuY2VDbGllbnRDb25maWd1cmF0aW9uXG4gICAgICogQGRlc2MgSXQgd2lsbCBiZSB1c2VkIGZvciBjcmVhdGluZyBXZWJUcmFuc3BvcnQuXG4gICAgICogQHNlZSB7QGxpbmsgaHR0cHM6Ly93M2MuZ2l0aHViLmlvL3dlYnRyYW5zcG9ydC8jZGljdGRlZi13ZWJ0cmFuc3BvcnRvcHRpb25zfFdlYlRyYW5zcG9ydE9wdGlvbnMgb2YgV2ViVHJhbnNwb3J0fS5cbiAgICAgKiBAZXhhbXBsZVxuICAgICAqIC8vIEZvbGxvd2luZyBvYmplY3QgY2FuIGJlIHNldCB0byBjb25mZXJlbmNlQ2xpZW50Q29uZmlndXJhdGlvbi53ZWJUcmFuc3BvcnRDb25maWd1cmF0aW9uLlxuICAgICAqIHtcbiAgICAgKiAgIHNlcnZlckNlcnRpZmljYXRlRmluZ2VycHJpbnRzOiBbe1xuICAgICAqICAgICB2YWx1ZTpcbiAgICAgKiAgICAgICAgICcwMDoxMToyMjozMzo0NDo1NTo2Njo3Nzo4ODo5OTpBQTpCQjpDQzpERDpFRTpGRjowMDoxMToyMjozMzo0NDo1NTo2Njo3Nzo4ODo5OTpBQTpCQjpDQzpERDpFRTpGRicsXG4gICAgICogICAgIGFsZ29yaXRobTogJ3NoYS0yNTYnLFxuICAgICAqICAgfV0sXG4gICAgICogfVxuICAgICAqL1xuICAgIHRoaXMud2ViVHJhbnNwb3J0Q29uZmlndXJhdGlvbiA9IHVuZGVmaW5lZDtcbiAgfVxufVxuXG4vKipcbiAqIEBjbGFzcyBDb25mZXJlbmNlQ2xpZW50XG4gKiBAY2xhc3NkZXNjIFRoZSBDb25mZXJlbmNlQ2xpZW50IGhhbmRsZXMgUGVlckNvbm5lY3Rpb25zIGJldHdlZW4gY2xpZW50IGFuZCBzZXJ2ZXIuIEZvciBjb25mZXJlbmNlIGNvbnRyb2xsaW5nLCBwbGVhc2UgcmVmZXIgdG8gUkVTVCBBUEkgZ3VpZGUuXG4gKiBFdmVudHM6XG4gKlxuICogfCBFdmVudCBOYW1lICAgICAgICAgICAgfCBBcmd1bWVudCBUeXBlICAgICAgICAgICAgICAgICAgICB8IEZpcmVkIHdoZW4gICAgICAgfFxuICogfCAtLS0tLS0tLS0tLS0tLS0tLS0tLS0gfCAtLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS18IC0tLS0tLS0tLS0tLS0tLS0gfFxuICogfCBzdHJlYW1hZGRlZCAgICAgICAgICAgfCBPd3QuQmFzZS5TdHJlYW1FdmVudCAgICAgICAgICAgICB8IEEgbmV3IHN0cmVhbSBpcyBhdmFpbGFibGUgaW4gdGhlIGNvbmZlcmVuY2UuIHxcbiAqIHwgcGFydGljaXBhbnRqb2luZWQgICAgIHwgT3d0LkNvbmZlcmVuY2UuUGFydGljaXBhbnRFdmVudCAgfCBBIG5ldyBwYXJ0aWNpcGFudCBqb2luZWQgdGhlIGNvbmZlcmVuY2UuIHxcbiAqIHwgbWVzc2FnZXJlY2VpdmVkICAgICAgIHwgT3d0LkJhc2UuTWVzc2FnZUV2ZW50ICAgICAgICAgICAgfCBBIG5ldyBtZXNzYWdlIGlzIHJlY2VpdmVkLiB8XG4gKiB8IHNlcnZlcmRpc2Nvbm5lY3RlZCAgICB8IE93dC5CYXNlLk93dEV2ZW50ICAgICAgICAgICAgICAgIHwgRGlzY29ubmVjdGVkIGZyb20gY29uZmVyZW5jZSBzZXJ2ZXIuIHxcbiAqXG4gKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2VcbiAqIEBleHRlbmRzIE93dC5CYXNlLkV2ZW50RGlzcGF0Y2hlclxuICogQGNvbnN0cnVjdG9yXG4gKiBAcGFyYW0gez9Pd3QuQ29uZmVyZW5jZS5Db25mZXJlbmNlQ2xpZW50Q29uZmlndXJhdGlvbiB9IGNvbmZpZyBDb25maWd1cmF0aW9uIGZvciBDb25mZXJlbmNlQ2xpZW50LlxuICogQHBhcmFtIHs/T3d0LkNvbmZlcmVuY2UuU2lvU2lnbmFsaW5nIH0gc2lnbmFsaW5nSW1wbCBTaWduYWxpbmcgY2hhbm5lbCBpbXBsZW1lbnRhdGlvbiBmb3IgQ29uZmVyZW5jZUNsaWVudC4gU0RLIHVzZXMgZGVmYXVsdCBzaWduYWxpbmcgY2hhbm5lbCBpbXBsZW1lbnRhdGlvbiBpZiB0aGlzIHBhcmFtZXRlciBpcyB1bmRlZmluZWQuIEN1cnJlbnRseSwgYSBTb2NrZXQuSU8gc2lnbmFsaW5nIGNoYW5uZWwgaW1wbGVtZW50YXRpb24gd2FzIHByb3ZpZGVkIGFzIGljcy5jb25mZXJlbmNlLlNpb1NpZ25hbGluZy4gSG93ZXZlciwgaXQgaXMgbm90IHJlY29tbWVuZGVkIHRvIGRpcmVjdGx5IGFjY2VzcyBzaWduYWxpbmcgY2hhbm5lbCBvciBjdXN0b21pemUgc2lnbmFsaW5nIGNoYW5uZWwgZm9yIENvbmZlcmVuY2VDbGllbnQgYXMgdGhpcyB0aW1lLlxuICovXG5leHBvcnQgY29uc3QgQ29uZmVyZW5jZUNsaWVudCA9IGZ1bmN0aW9uKGNvbmZpZywgc2lnbmFsaW5nSW1wbCkge1xuICBPYmplY3Quc2V0UHJvdG90eXBlT2YodGhpcywgbmV3IEV2ZW50TW9kdWxlLkV2ZW50RGlzcGF0Y2hlcigpKTtcbiAgY29uZmlnID0gY29uZmlnIHx8IHt9O1xuICBjb25zdCBzZWxmID0gdGhpcztcbiAgbGV0IHNpZ25hbGluZ1N0YXRlID0gU2lnbmFsaW5nU3RhdGUuUkVBRFk7XG4gIGNvbnN0IHNpZ25hbGluZyA9IHNpZ25hbGluZ0ltcGwgPyBzaWduYWxpbmdJbXBsIDogKG5ldyBTaWduYWxpbmcoKSk7XG4gIGxldCBtZTtcbiAgbGV0IHJvb207XG4gIGNvbnN0IHJlbW90ZVN0cmVhbXMgPSBuZXcgTWFwKCk7IC8vIEtleSBpcyBzdHJlYW0gSUQsIHZhbHVlIGlzIGEgUmVtb3RlU3RyZWFtLlxuICBjb25zdCBwYXJ0aWNpcGFudHMgPSBuZXcgTWFwKCk7IC8vIEtleSBpcyBwYXJ0aWNpcGFudCBJRCwgdmFsdWUgaXMgYSBQYXJ0aWNpcGFudCBvYmplY3QuXG4gIGNvbnN0IHB1Ymxpc2hDaGFubmVscyA9IG5ldyBNYXAoKTsgLy8gS2V5IGlzIE1lZGlhU3RyZWFtJ3MgSUQsIHZhbHVlIGlzIHBjIGNoYW5uZWwuXG4gIGNvbnN0IGNoYW5uZWxzID0gbmV3IE1hcCgpOyAvLyBLZXkgaXMgY2hhbm5lbCdzIGludGVybmFsIElELCB2YWx1ZSBpcyBjaGFubmVsLlxuICBsZXQgcGVlckNvbm5lY3Rpb25DaGFubmVsID0gbnVsbDsgLy8gUGVlckNvbm5lY3Rpb24gZm9yIFdlYlJUQy5cbiAgbGV0IHF1aWNUcmFuc3BvcnRDaGFubmVsID0gbnVsbDtcblxuICAvKipcbiAgICogQG1lbWJlciB7UlRDUGVlckNvbm5lY3Rpb259IHBlZXJDb25uZWN0aW9uXG4gICAqIEBpbnN0YW5jZVxuICAgKiBAcmVhZG9ubHlcbiAgICogQGRlc2MgUGVlckNvbm5lY3Rpb24gZm9yIFdlYlJUQyBjb25uZWN0aW9uIHdpdGggY29uZmVyZW5jZSBzZXJ2ZXIuXG4gICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5Db25mZXJlbmNlQ2xpZW50XG4gICAqIEBzZWUge0BsaW5rIGh0dHBzOi8vdzNjLmdpdGh1Yi5pby93ZWJydGMtcGMvI3J0Y3BlZXJjb25uZWN0aW9uLWludGVyZmFjZXxSVENQZWVyQ29ubmVjdGlvbiBJbnRlcmZhY2Ugb2YgV2ViUlRDIDEuMH0uXG4gICAqL1xuICBPYmplY3QuZGVmaW5lUHJvcGVydHkodGhpcywgJ3BlZXJDb25uZWN0aW9uJywge1xuICAgIGNvbmZpZ3VyYWJsZTogZmFsc2UsXG4gICAgZ2V0KCkge1xuICAgICAgcmV0dXJuIHBlZXJDb25uZWN0aW9uQ2hhbm5lbD8ucGM7XG4gICAgfSxcbiAgfSk7XG5cbiAgLyoqXG4gICAqIEBmdW5jdGlvbiBvblNpZ25hbGluZ01lc3NhZ2VcbiAgICogQGRlc2MgUmVjZWl2ZWQgYSBtZXNzYWdlIGZyb20gY29uZmVyZW5jZSBwb3J0YWwuIERlZmluZWQgaW4gY2xpZW50LXNlcnZlciBwcm90b2NvbC5cbiAgICogQHBhcmFtIHtzdHJpbmd9IG5vdGlmaWNhdGlvbiBOb3RpZmljYXRpb24gdHlwZS5cbiAgICogQHBhcmFtIHtvYmplY3R9IGRhdGEgRGF0YSByZWNlaXZlZC5cbiAgICogQHByaXZhdGVcbiAgICovXG4gIGZ1bmN0aW9uIG9uU2lnbmFsaW5nTWVzc2FnZShub3RpZmljYXRpb24sIGRhdGEpIHtcbiAgICBpZiAobm90aWZpY2F0aW9uID09PSAnc29hYycgfHwgbm90aWZpY2F0aW9uID09PSAncHJvZ3Jlc3MnKSB7XG4gICAgICBpZiAoY2hhbm5lbHMuaGFzKGRhdGEuaWQpKSB7XG4gICAgICAgIGNoYW5uZWxzLmdldChkYXRhLmlkKS5vbk1lc3NhZ2Uobm90aWZpY2F0aW9uLCBkYXRhKTtcbiAgICAgIH0gZWxzZSBpZiAocXVpY1RyYW5zcG9ydENoYW5uZWwgJiYgcXVpY1RyYW5zcG9ydENoYW5uZWxcbiAgICAgICAgICAuaGFzQ29udGVudFNlc3Npb25JZChkYXRhLmlkKSkge1xuICAgICAgICBxdWljVHJhbnNwb3J0Q2hhbm5lbC5vbk1lc3NhZ2Uobm90aWZpY2F0aW9uLCBkYXRhKTtcbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIExvZ2dlci53YXJuaW5nKCdDYW5ub3QgZmluZCBhIGNoYW5uZWwgZm9yIGluY29taW5nIGRhdGEuJyk7XG4gICAgICB9XG4gICAgICByZXR1cm47XG4gICAgfSBlbHNlIGlmIChub3RpZmljYXRpb24gPT09ICdzdHJlYW0nKSB7XG4gICAgICBpZiAoZGF0YS5zdGF0dXMgPT09ICdhZGQnKSB7XG4gICAgICAgIGZpcmVTdHJlYW1BZGRlZChkYXRhLmRhdGEpO1xuICAgICAgfSBlbHNlIGlmIChkYXRhLnN0YXR1cyA9PT0gJ3JlbW92ZScpIHtcbiAgICAgICAgZmlyZVN0cmVhbVJlbW92ZWQoZGF0YSk7XG4gICAgICB9IGVsc2UgaWYgKGRhdGEuc3RhdHVzID09PSAndXBkYXRlJykge1xuICAgICAgICAvLyBCcm9hZGNhc3QgYXVkaW8vdmlkZW8gdXBkYXRlIHN0YXR1cyB0byBjaGFubmVsIHNvIHNwZWNpZmljIGV2ZW50cyBjYW4gYmUgZmlyZWQgb24gcHVibGljYXRpb24gb3Igc3Vic2NyaXB0aW9uLlxuICAgICAgICBpZiAoZGF0YS5kYXRhLmZpZWxkID09PSAnYXVkaW8uc3RhdHVzJyB8fCBkYXRhLmRhdGEuZmllbGQgPT09XG4gICAgICAgICAgJ3ZpZGVvLnN0YXR1cycpIHtcbiAgICAgICAgICBjaGFubmVscy5mb3JFYWNoKChjKSA9PiB7XG4gICAgICAgICAgICBjLm9uTWVzc2FnZShub3RpZmljYXRpb24sIGRhdGEpO1xuICAgICAgICAgIH0pO1xuICAgICAgICB9IGVsc2UgaWYgKGRhdGEuZGF0YS5maWVsZCA9PT0gJ2FjdGl2ZUlucHV0Jykge1xuICAgICAgICAgIGZpcmVBY3RpdmVBdWRpb0lucHV0Q2hhbmdlKGRhdGEpO1xuICAgICAgICB9IGVsc2UgaWYgKGRhdGEuZGF0YS5maWVsZCA9PT0gJ3ZpZGVvLmxheW91dCcpIHtcbiAgICAgICAgICBmaXJlTGF5b3V0Q2hhbmdlKGRhdGEpO1xuICAgICAgICB9IGVsc2UgaWYgKGRhdGEuZGF0YS5maWVsZCA9PT0gJy4nKSB7XG4gICAgICAgICAgdXBkYXRlUmVtb3RlU3RyZWFtKGRhdGEuZGF0YS52YWx1ZSk7XG4gICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgTG9nZ2VyLndhcm5pbmcoJ1Vua25vd24gc3RyZWFtIGV2ZW50IGZyb20gTUNVLicpO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgfSBlbHNlIGlmIChub3RpZmljYXRpb24gPT09ICd0ZXh0Jykge1xuICAgICAgZmlyZU1lc3NhZ2VSZWNlaXZlZChkYXRhKTtcbiAgICB9IGVsc2UgaWYgKG5vdGlmaWNhdGlvbiA9PT0gJ3BhcnRpY2lwYW50Jykge1xuICAgICAgZmlyZVBhcnRpY2lwYW50RXZlbnQoZGF0YSk7XG4gICAgfVxuICB9XG5cbiAgc2lnbmFsaW5nLmFkZEV2ZW50TGlzdGVuZXIoJ2RhdGEnLCAoZXZlbnQpID0+IHtcbiAgICBvblNpZ25hbGluZ01lc3NhZ2UoZXZlbnQubWVzc2FnZS5ub3RpZmljYXRpb24sIGV2ZW50Lm1lc3NhZ2UuZGF0YSk7XG4gIH0pO1xuXG4gIHNpZ25hbGluZy5hZGRFdmVudExpc3RlbmVyKCdkaXNjb25uZWN0JywgKCkgPT4ge1xuICAgIGNsZWFuKCk7XG4gICAgc2lnbmFsaW5nU3RhdGUgPSBTaWduYWxpbmdTdGF0ZS5SRUFEWTtcbiAgICBzZWxmLmRpc3BhdGNoRXZlbnQobmV3IEV2ZW50TW9kdWxlLk93dEV2ZW50KCdzZXJ2ZXJkaXNjb25uZWN0ZWQnKSk7XG4gIH0pO1xuXG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGZ1bmN0aW9uIGZpcmVQYXJ0aWNpcGFudEV2ZW50KGRhdGEpIHtcbiAgICBpZiAoZGF0YS5hY3Rpb24gPT09ICdqb2luJykge1xuICAgICAgZGF0YSA9IGRhdGEuZGF0YTtcbiAgICAgIGNvbnN0IHBhcnRpY2lwYW50ID0gbmV3IFBhcnRpY2lwYW50KGRhdGEuaWQsIGRhdGEucm9sZSwgZGF0YS51c2VyKTtcbiAgICAgIHBhcnRpY2lwYW50cy5zZXQoZGF0YS5pZCwgcGFydGljaXBhbnQpO1xuICAgICAgY29uc3QgZXZlbnQgPSBuZXcgUGFydGljaXBhbnRFdmVudChcbiAgICAgICAgICAncGFydGljaXBhbnRqb2luZWQnLCB7cGFydGljaXBhbnQ6IHBhcnRpY2lwYW50fSk7XG4gICAgICBzZWxmLmRpc3BhdGNoRXZlbnQoZXZlbnQpO1xuICAgIH0gZWxzZSBpZiAoZGF0YS5hY3Rpb24gPT09ICdsZWF2ZScpIHtcbiAgICAgIGNvbnN0IHBhcnRpY2lwYW50SWQgPSBkYXRhLmRhdGE7XG4gICAgICBpZiAoIXBhcnRpY2lwYW50cy5oYXMocGFydGljaXBhbnRJZCkpIHtcbiAgICAgICAgTG9nZ2VyLndhcm5pbmcoXG4gICAgICAgICAgICAnUmVjZWl2ZWQgbGVhdmUgbWVzc2FnZSBmcm9tIE1DVSBmb3IgYW4gdW5rbm93biBwYXJ0aWNpcGFudC4nKTtcbiAgICAgICAgcmV0dXJuO1xuICAgICAgfVxuICAgICAgY29uc3QgcGFydGljaXBhbnQgPSBwYXJ0aWNpcGFudHMuZ2V0KHBhcnRpY2lwYW50SWQpO1xuICAgICAgcGFydGljaXBhbnRzLmRlbGV0ZShwYXJ0aWNpcGFudElkKTtcbiAgICAgIHBhcnRpY2lwYW50LmRpc3BhdGNoRXZlbnQobmV3IEV2ZW50TW9kdWxlLk93dEV2ZW50KCdsZWZ0JykpO1xuICAgIH1cbiAgfVxuXG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGZ1bmN0aW9uIGZpcmVNZXNzYWdlUmVjZWl2ZWQoZGF0YSkge1xuICAgIGNvbnN0IG1lc3NhZ2VFdmVudCA9IG5ldyBFdmVudE1vZHVsZS5NZXNzYWdlRXZlbnQoJ21lc3NhZ2VyZWNlaXZlZCcsIHtcbiAgICAgIG1lc3NhZ2U6IGRhdGEubWVzc2FnZSxcbiAgICAgIG9yaWdpbjogZGF0YS5mcm9tLFxuICAgICAgdG86IGRhdGEudG8sXG4gICAgfSk7XG4gICAgc2VsZi5kaXNwYXRjaEV2ZW50KG1lc3NhZ2VFdmVudCk7XG4gIH1cblxuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBmdW5jdGlvbiBmaXJlU3RyZWFtQWRkZWQoaW5mbykge1xuICAgIGNvbnN0IHN0cmVhbSA9IGNyZWF0ZVJlbW90ZVN0cmVhbShpbmZvKTtcbiAgICByZW1vdGVTdHJlYW1zLnNldChzdHJlYW0uaWQsIHN0cmVhbSk7XG4gICAgY29uc3Qgc3RyZWFtRXZlbnQgPSBuZXcgU3RyZWFtTW9kdWxlLlN0cmVhbUV2ZW50KCdzdHJlYW1hZGRlZCcsIHtcbiAgICAgIHN0cmVhbTogc3RyZWFtLFxuICAgIH0pO1xuICAgIHNlbGYuZGlzcGF0Y2hFdmVudChzdHJlYW1FdmVudCk7XG4gIH1cblxuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBmdW5jdGlvbiBmaXJlU3RyZWFtUmVtb3ZlZChpbmZvKSB7XG4gICAgaWYgKCFyZW1vdGVTdHJlYW1zLmhhcyhpbmZvLmlkKSkge1xuICAgICAgTG9nZ2VyLndhcm5pbmcoJ0Nhbm5vdCBmaW5kIHNwZWNpZmljIHJlbW90ZSBzdHJlYW0uJyk7XG4gICAgICByZXR1cm47XG4gICAgfVxuICAgIGNvbnN0IHN0cmVhbSA9IHJlbW90ZVN0cmVhbXMuZ2V0KGluZm8uaWQpO1xuICAgIGNvbnN0IHN0cmVhbUV2ZW50ID0gbmV3IEV2ZW50TW9kdWxlLk93dEV2ZW50KCdlbmRlZCcpO1xuICAgIHJlbW90ZVN0cmVhbXMuZGVsZXRlKHN0cmVhbS5pZCk7XG4gICAgc3RyZWFtLmRpc3BhdGNoRXZlbnQoc3RyZWFtRXZlbnQpO1xuICB9XG5cbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgZnVuY3Rpb24gZmlyZUFjdGl2ZUF1ZGlvSW5wdXRDaGFuZ2UoaW5mbykge1xuICAgIGlmICghcmVtb3RlU3RyZWFtcy5oYXMoaW5mby5pZCkpIHtcbiAgICAgIExvZ2dlci53YXJuaW5nKCdDYW5ub3QgZmluZCBzcGVjaWZpYyByZW1vdGUgc3RyZWFtLicpO1xuICAgICAgcmV0dXJuO1xuICAgIH1cbiAgICBjb25zdCBzdHJlYW0gPSByZW1vdGVTdHJlYW1zLmdldChpbmZvLmlkKTtcbiAgICBjb25zdCBzdHJlYW1FdmVudCA9IG5ldyBBY3RpdmVBdWRpb0lucHV0Q2hhbmdlRXZlbnQoXG4gICAgICAgICdhY3RpdmVhdWRpb2lucHV0Y2hhbmdlJywge1xuICAgICAgICAgIGFjdGl2ZUF1ZGlvSW5wdXRTdHJlYW1JZDogaW5mby5kYXRhLnZhbHVlLFxuICAgICAgICB9KTtcbiAgICBzdHJlYW0uZGlzcGF0Y2hFdmVudChzdHJlYW1FdmVudCk7XG4gIH1cblxuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBmdW5jdGlvbiBmaXJlTGF5b3V0Q2hhbmdlKGluZm8pIHtcbiAgICBpZiAoIXJlbW90ZVN0cmVhbXMuaGFzKGluZm8uaWQpKSB7XG4gICAgICBMb2dnZXIud2FybmluZygnQ2Fubm90IGZpbmQgc3BlY2lmaWMgcmVtb3RlIHN0cmVhbS4nKTtcbiAgICAgIHJldHVybjtcbiAgICB9XG4gICAgY29uc3Qgc3RyZWFtID0gcmVtb3RlU3RyZWFtcy5nZXQoaW5mby5pZCk7XG4gICAgY29uc3Qgc3RyZWFtRXZlbnQgPSBuZXcgTGF5b3V0Q2hhbmdlRXZlbnQoXG4gICAgICAgICdsYXlvdXRjaGFuZ2UnLCB7XG4gICAgICAgICAgbGF5b3V0OiBpbmZvLmRhdGEudmFsdWUsXG4gICAgICAgIH0pO1xuICAgIHN0cmVhbS5kaXNwYXRjaEV2ZW50KHN0cmVhbUV2ZW50KTtcbiAgfVxuXG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGZ1bmN0aW9uIHVwZGF0ZVJlbW90ZVN0cmVhbShzdHJlYW1JbmZvKSB7XG4gICAgaWYgKCFyZW1vdGVTdHJlYW1zLmhhcyhzdHJlYW1JbmZvLmlkKSkge1xuICAgICAgTG9nZ2VyLndhcm5pbmcoJ0Nhbm5vdCBmaW5kIHNwZWNpZmljIHJlbW90ZSBzdHJlYW0uJyk7XG4gICAgICByZXR1cm47XG4gICAgfVxuICAgIGNvbnN0IHN0cmVhbSA9IHJlbW90ZVN0cmVhbXMuZ2V0KHN0cmVhbUluZm8uaWQpO1xuICAgIHN0cmVhbS5zZXR0aW5ncyA9IFN0cmVhbVV0aWxzTW9kdWxlLmNvbnZlcnRUb1B1YmxpY2F0aW9uU2V0dGluZ3Moc3RyZWFtSW5mb1xuICAgICAgICAubWVkaWEpO1xuICAgIHN0cmVhbS5leHRyYUNhcGFiaWxpdGllcyA9IFN0cmVhbVV0aWxzTW9kdWxlXG4gICAgICAgIC5jb252ZXJ0VG9TdWJzY3JpcHRpb25DYXBhYmlsaXRpZXMoXG4gICAgICAgICAgICBzdHJlYW1JbmZvLm1lZGlhKTtcbiAgICBjb25zdCBzdHJlYW1FdmVudCA9IG5ldyBFdmVudE1vZHVsZS5Pd3RFdmVudCgndXBkYXRlZCcpO1xuICAgIHN0cmVhbS5kaXNwYXRjaEV2ZW50KHN0cmVhbUV2ZW50KTtcbiAgfVxuXG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGZ1bmN0aW9uIGNyZWF0ZVJlbW90ZVN0cmVhbShzdHJlYW1JbmZvKSB7XG4gICAgaWYgKHN0cmVhbUluZm8udHlwZSA9PT0gJ21peGVkJykge1xuICAgICAgcmV0dXJuIG5ldyBSZW1vdGVNaXhlZFN0cmVhbShzdHJlYW1JbmZvKTtcbiAgICB9IGVsc2Uge1xuICAgICAgbGV0IGF1ZGlvU291cmNlSW5mbzsgbGV0IHZpZGVvU291cmNlSW5mbztcbiAgICAgIGNvbnN0IGF1ZGlvVHJhY2sgPSBzdHJlYW1JbmZvLm1lZGlhLnRyYWNrc1xuICAgICAgICAgIC5maW5kKCh0KSA9PiB0LnR5cGUgPT09ICdhdWRpbycpO1xuICAgICAgY29uc3QgdmlkZW9UcmFjayA9IHN0cmVhbUluZm8ubWVkaWEudHJhY2tzXG4gICAgICAgICAgLmZpbmQoKHQpID0+IHQudHlwZSA9PT0gJ3ZpZGVvJyk7XG4gICAgICBpZiAoYXVkaW9UcmFjaykge1xuICAgICAgICBhdWRpb1NvdXJjZUluZm8gPSBhdWRpb1RyYWNrLnNvdXJjZTtcbiAgICAgIH1cbiAgICAgIGlmICh2aWRlb1RyYWNrKSB7XG4gICAgICAgIHZpZGVvU291cmNlSW5mbyA9IHZpZGVvVHJhY2suc291cmNlO1xuICAgICAgfVxuICAgICAgY29uc3QgZGF0YVNvdXJjZUluZm8gPSBzdHJlYW1JbmZvLmRhdGE7XG4gICAgICBjb25zdCBzdHJlYW0gPSBuZXcgU3RyZWFtTW9kdWxlLlJlbW90ZVN0cmVhbShzdHJlYW1JbmZvLmlkLFxuICAgICAgICAgIHN0cmVhbUluZm8uaW5mby5vd25lciwgdW5kZWZpbmVkLCBuZXcgU3RyZWFtTW9kdWxlLlN0cmVhbVNvdXJjZUluZm8oXG4gICAgICAgICAgICAgIGF1ZGlvU291cmNlSW5mbywgdmlkZW9Tb3VyY2VJbmZvLCBkYXRhU291cmNlSW5mbyksIHN0cmVhbUluZm8uaW5mb1xuICAgICAgICAgICAgICAuYXR0cmlidXRlcyk7XG4gICAgICBzdHJlYW0uc2V0dGluZ3MgPSBTdHJlYW1VdGlsc01vZHVsZS5jb252ZXJ0VG9QdWJsaWNhdGlvblNldHRpbmdzKFxuICAgICAgICAgIHN0cmVhbUluZm8ubWVkaWEpO1xuICAgICAgc3RyZWFtLmV4dHJhQ2FwYWJpbGl0aWVzID0gU3RyZWFtVXRpbHNNb2R1bGVcbiAgICAgICAgICAuY29udmVydFRvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzKFxuICAgICAgICAgICAgICBzdHJlYW1JbmZvLm1lZGlhKTtcbiAgICAgIHJldHVybiBzdHJlYW07XG4gICAgfVxuICB9XG5cbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgZnVuY3Rpb24gc2VuZFNpZ25hbGluZ01lc3NhZ2UodHlwZSwgbWVzc2FnZSkge1xuICAgIHJldHVybiBzaWduYWxpbmcuc2VuZCh0eXBlLCBtZXNzYWdlKTtcbiAgfVxuXG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGZ1bmN0aW9uIGNyZWF0ZVNpZ25hbGluZ0ZvckNoYW5uZWwoKSB7XG4gICAgLy8gQ29uc3RydWN0IGFuIHNpZ25hbGluZyBzZW5kZXIvcmVjZWl2ZXIgZm9yIENvbmZlcmVuY2VQZWVyQ29ubmVjdGlvbi5cbiAgICBjb25zdCBzaWduYWxpbmdGb3JDaGFubmVsID0gT2JqZWN0LmNyZWF0ZShFdmVudE1vZHVsZS5FdmVudERpc3BhdGNoZXIpO1xuICAgIHNpZ25hbGluZ0ZvckNoYW5uZWwuc2VuZFNpZ25hbGluZ01lc3NhZ2UgPSBzZW5kU2lnbmFsaW5nTWVzc2FnZTtcbiAgICByZXR1cm4gc2lnbmFsaW5nRm9yQ2hhbm5lbDtcbiAgfVxuXG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGZ1bmN0aW9uIGNyZWF0ZVBlZXJDb25uZWN0aW9uQ2hhbm5lbCh0cmFuc3BvcnQpIHtcbiAgICBjb25zdCBzaWduYWxpbmdGb3JDaGFubmVsID0gY3JlYXRlU2lnbmFsaW5nRm9yQ2hhbm5lbCgpO1xuICAgIGNvbnN0IGNoYW5uZWwgPVxuICAgICAgICBuZXcgQ29uZmVyZW5jZVBlZXJDb25uZWN0aW9uQ2hhbm5lbChjb25maWcsIHNpZ25hbGluZ0ZvckNoYW5uZWwpO1xuICAgIGNoYW5uZWwuYWRkRXZlbnRMaXN0ZW5lcignaWQnLCAobWVzc2FnZUV2ZW50KSA9PiB7XG4gICAgICBpZiAoIWNoYW5uZWxzLmhhcyhtZXNzYWdlRXZlbnQubWVzc2FnZSkpIHtcbiAgICAgICAgY2hhbm5lbHMuc2V0KG1lc3NhZ2VFdmVudC5tZXNzYWdlLCBjaGFubmVsKTtcbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIExvZ2dlci53YXJuaW5nKCdDaGFubmVsIGFscmVhZHkgZXhpc3RzJywgbWVzc2FnZUV2ZW50Lm1lc3NhZ2UpO1xuICAgICAgfVxuICAgIH0pO1xuICAgIHJldHVybiBjaGFubmVsO1xuICB9XG5cbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgZnVuY3Rpb24gY2xlYW4oKSB7XG4gICAgcGFydGljaXBhbnRzLmNsZWFyKCk7XG4gICAgcmVtb3RlU3RyZWFtcy5jbGVhcigpO1xuICB9XG5cbiAgT2JqZWN0LmRlZmluZVByb3BlcnR5KHRoaXMsICdpbmZvJywge1xuICAgIGNvbmZpZ3VyYWJsZTogZmFsc2UsXG4gICAgZ2V0OiAoKSA9PiB7XG4gICAgICBpZiAoIXJvb20pIHtcbiAgICAgICAgcmV0dXJuIG51bGw7XG4gICAgICB9XG4gICAgICByZXR1cm4gbmV3IENvbmZlcmVuY2VJbmZvKHJvb20uaWQsIEFycmF5LmZyb20ocGFydGljaXBhbnRzLCAoeCkgPT4geFtcbiAgICAgICAgICAxXSksIEFycmF5LmZyb20ocmVtb3RlU3RyZWFtcywgKHgpID0+IHhbMV0pLCBtZSk7XG4gICAgfSxcbiAgfSk7XG5cbiAgLyoqXG4gICAqIEBmdW5jdGlvbiBqb2luXG4gICAqIEBpbnN0YW5jZVxuICAgKiBAZGVzYyBKb2luIGEgY29uZmVyZW5jZS5cbiAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLkNvbmZlcmVuY2VDbGllbnRcbiAgICogQHJldHVybiB7UHJvbWlzZTxDb25mZXJlbmNlSW5mbywgRXJyb3I+fSBSZXR1cm4gYSBwcm9taXNlIHJlc29sdmVkIHdpdGggY3VycmVudCBjb25mZXJlbmNlJ3MgaW5mb3JtYXRpb24gaWYgc3VjY2Vzc2Z1bGx5IGpvaW4gdGhlIGNvbmZlcmVuY2UuIE9yIHJldHVybiBhIHByb21pc2UgcmVqZWN0ZWQgd2l0aCBhIG5ld2x5IGNyZWF0ZWQgT3d0LkVycm9yIGlmIGZhaWxlZCB0byBqb2luIHRoZSBjb25mZXJlbmNlLlxuICAgKiBAcGFyYW0ge3N0cmluZ30gdG9rZW5TdHJpbmcgVG9rZW4gaXMgaXNzdWVkIGJ5IGNvbmZlcmVuY2Ugc2VydmVyKG51dmUpLlxuICAgKi9cbiAgdGhpcy5qb2luID0gZnVuY3Rpb24odG9rZW5TdHJpbmcpIHtcbiAgICByZXR1cm4gbmV3IFByb21pc2UoKHJlc29sdmUsIHJlamVjdCkgPT4ge1xuICAgICAgY29uc3QgdG9rZW4gPSBKU09OLnBhcnNlKEJhc2U2NC5kZWNvZGVCYXNlNjQodG9rZW5TdHJpbmcpKTtcbiAgICAgIGNvbnN0IGlzU2VjdXJlZCA9ICh0b2tlbi5zZWN1cmUgPT09IHRydWUpO1xuICAgICAgbGV0IGhvc3QgPSB0b2tlbi5ob3N0O1xuICAgICAgaWYgKHR5cGVvZiBob3N0ICE9PSAnc3RyaW5nJykge1xuICAgICAgICByZWplY3QobmV3IENvbmZlcmVuY2VFcnJvcignSW52YWxpZCBob3N0LicpKTtcbiAgICAgICAgcmV0dXJuO1xuICAgICAgfVxuICAgICAgaWYgKGhvc3QuaW5kZXhPZignaHR0cCcpID09PSAtMSkge1xuICAgICAgICBob3N0ID0gaXNTZWN1cmVkID8gKCdodHRwczovLycgKyBob3N0KSA6ICgnaHR0cDovLycgKyBob3N0KTtcbiAgICAgIH1cbiAgICAgIGlmIChzaWduYWxpbmdTdGF0ZSAhPT0gU2lnbmFsaW5nU3RhdGUuUkVBRFkpIHtcbiAgICAgICAgcmVqZWN0KG5ldyBDb25mZXJlbmNlRXJyb3IoJ2Nvbm5lY3Rpb24gc3RhdGUgaW52YWxpZCcpKTtcbiAgICAgICAgcmV0dXJuO1xuICAgICAgfVxuXG4gICAgICBzaWduYWxpbmdTdGF0ZSA9IFNpZ25hbGluZ1N0YXRlLkNPTk5FQ1RJTkc7XG5cbiAgICAgIGNvbnN0IHN5c0luZm8gPSBVdGlscy5zeXNJbmZvKCk7XG4gICAgICBjb25zdCBsb2dpbkluZm8gPSB7XG4gICAgICAgIHRva2VuOiB0b2tlblN0cmluZyxcbiAgICAgICAgdXNlckFnZW50OiB7c2RrOiBzeXNJbmZvLnNka30sXG4gICAgICAgIHByb3RvY29sOiBwcm90b2NvbFZlcnNpb24sXG4gICAgICB9O1xuXG4gICAgICBzaWduYWxpbmcuY29ubmVjdChob3N0LCBpc1NlY3VyZWQsIGxvZ2luSW5mbykudGhlbigocmVzcCkgPT4ge1xuICAgICAgICBzaWduYWxpbmdTdGF0ZSA9IFNpZ25hbGluZ1N0YXRlLkNPTk5FQ1RFRDtcbiAgICAgICAgcm9vbSA9IHJlc3Aucm9vbTtcbiAgICAgICAgaWYgKHJvb20uc3RyZWFtcyAhPT0gdW5kZWZpbmVkKSB7XG4gICAgICAgICAgZm9yIChjb25zdCBzdCBvZiByb29tLnN0cmVhbXMpIHtcbiAgICAgICAgICAgIGlmIChzdC50eXBlID09PSAnbWl4ZWQnKSB7XG4gICAgICAgICAgICAgIHN0LnZpZXdwb3J0ID0gc3QuaW5mby5sYWJlbDtcbiAgICAgICAgICAgIH1cbiAgICAgICAgICAgIHJlbW90ZVN0cmVhbXMuc2V0KHN0LmlkLCBjcmVhdGVSZW1vdGVTdHJlYW0oc3QpKTtcbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgICAgaWYgKHJlc3Aucm9vbSAmJiByZXNwLnJvb20ucGFydGljaXBhbnRzICE9PSB1bmRlZmluZWQpIHtcbiAgICAgICAgICBmb3IgKGNvbnN0IHAgb2YgcmVzcC5yb29tLnBhcnRpY2lwYW50cykge1xuICAgICAgICAgICAgcGFydGljaXBhbnRzLnNldChwLmlkLCBuZXcgUGFydGljaXBhbnQocC5pZCwgcC5yb2xlLCBwLnVzZXIpKTtcbiAgICAgICAgICAgIGlmIChwLmlkID09PSByZXNwLmlkKSB7XG4gICAgICAgICAgICAgIG1lID0gcGFydGljaXBhbnRzLmdldChwLmlkKTtcbiAgICAgICAgICAgIH1cbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgICAgcGVlckNvbm5lY3Rpb25DaGFubmVsID0gY3JlYXRlUGVlckNvbm5lY3Rpb25DaGFubmVsKCk7XG4gICAgICAgIHBlZXJDb25uZWN0aW9uQ2hhbm5lbC5hZGRFdmVudExpc3RlbmVyKCdlbmRlZCcsICgpID0+IHtcbiAgICAgICAgICBwZWVyQ29ubmVjdGlvbkNoYW5uZWwgPSBudWxsO1xuICAgICAgICB9KTtcbiAgICAgICAgaWYgKHR5cGVvZiBXZWJUcmFuc3BvcnQgPT09ICdmdW5jdGlvbicgJiYgdG9rZW4ud2ViVHJhbnNwb3J0VXJsKSB7XG4gICAgICAgICAgcXVpY1RyYW5zcG9ydENoYW5uZWwgPSBuZXcgUXVpY0Nvbm5lY3Rpb24oXG4gICAgICAgICAgICAnaHR0cHM6Ly9qaWFuanVuei1udWMtdWJ1bnR1LnNoLmludGVsLmNvbTo3NzAwJywgcmVzcC53ZWJUcmFuc3BvcnRUb2tlbixcbiAgICAgICAgICAgICAgY3JlYXRlU2lnbmFsaW5nRm9yQ2hhbm5lbCgpLCBjb25maWcud2ViVHJhbnNwb3J0Q29uZmlndXJhdGlvbik7XG4gICAgICAgIH1cbiAgICAgICAgY29uc3QgY29uZmVyZW5jZUluZm8gPSBuZXcgQ29uZmVyZW5jZUluZm8oXG4gICAgICAgICAgICByZXNwLnJvb20uaWQsIEFycmF5LmZyb20ocGFydGljaXBhbnRzLnZhbHVlcygpKSxcbiAgICAgICAgICAgIEFycmF5LmZyb20ocmVtb3RlU3RyZWFtcy52YWx1ZXMoKSksIG1lKTtcbiAgICAgICAgaWYgKHF1aWNUcmFuc3BvcnRDaGFubmVsKSB7XG4gICAgICAgICAgcXVpY1RyYW5zcG9ydENoYW5uZWwuaW5pdCgpLnRoZW4oKCkgPT4ge1xuICAgICAgICAgICAgcmVzb2x2ZShjb25mZXJlbmNlSW5mbyk7XG4gICAgICAgICAgfSk7XG4gICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgcmVzb2x2ZShjb25mZXJlbmNlSW5mbyk7XG4gICAgICAgIH1cbiAgICAgIH0sIChlKSA9PiB7XG4gICAgICAgIHNpZ25hbGluZ1N0YXRlID0gU2lnbmFsaW5nU3RhdGUuUkVBRFk7XG4gICAgICAgIHJlamVjdChuZXcgQ29uZmVyZW5jZUVycm9yKGUpKTtcbiAgICAgIH0pO1xuICAgIH0pO1xuICB9O1xuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gcHVibGlzaFxuICAgKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2UuQ29uZmVyZW5jZUNsaWVudFxuICAgKiBAaW5zdGFuY2VcbiAgICogQGRlc2MgUHVibGlzaCBhIExvY2FsU3RyZWFtIHRvIGNvbmZlcmVuY2Ugc2VydmVyLiBPdGhlciBwYXJ0aWNpcGFudHMgd2lsbCBiZSBhYmxlIHRvIHN1YnNjcmliZSB0aGlzIHN0cmVhbSB3aGVuIGl0IGlzIHN1Y2Nlc3NmdWxseSBwdWJsaXNoZWQuXG4gICAqIEBwYXJhbSB7T3d0LkJhc2UuTG9jYWxTdHJlYW19IHN0cmVhbSBUaGUgc3RyZWFtIHRvIGJlIHB1Ymxpc2hlZC5cbiAgICogQHBhcmFtIHsoT3d0LkJhc2UuUHVibGlzaE9wdGlvbnN8UlRDUnRwVHJhbnNjZWl2ZXJbXSl9IG9wdGlvbnMgSWYgb3B0aW9ucyBpcyBhIFB1Ymxpc2hPcHRpb25zLCB0aGUgc3RyZWFtIHdpbGwgYmUgcHVibGlzaGVkIGFzIG9wdGlvbnMgc3BlY2lmaWVkLiBJZiBvcHRpb25zIGlzIGEgbGlzdCBvZiBSVENSdHBUcmFuc2NlaXZlcnMsIGVhY2ggdHJhY2sgaW4gdGhlIGZpcnN0IGFyZ3VtZW50IG11c3QgaGF2ZSBhIGNvcnJlc3BvbmRpbmcgUlRDUnRwVHJhbnNjZWl2ZXIgaGVyZSwgYW5kIHRoZSB0cmFjayB3aWxsIGJlIHB1Ymxpc2hlZCB3aXRoIHRoZSBSVENSdHBUcmFuc2NlaXZlciBhc3NvY2lhdGVkIHdpdGggaXQuXG4gICAqIEBwYXJhbSB7c3RyaW5nW119IHZpZGVvQ29kZWNzIFZpZGVvIGNvZGVjIG5hbWVzIGZvciBwdWJsaXNoaW5nLiBWYWxpZCB2YWx1ZXMgYXJlICdWUDgnLCAnVlA5JyBhbmQgJ0gyNjQnLiBUaGlzIHBhcmFtZXRlciBvbmx5IHZhbGlkIHdoZW4gdGhlIHNlY29uZCBhcmd1bWVudCBpcyBQdWJsaXNoT3B0aW9ucyBhbmQgb3B0aW9ucy52aWRlbyBpcyBSVENSdHBFbmNvZGluZ1BhcmFtZXRlcnMuIFB1Ymxpc2hpbmcgd2l0aCBSVENSdHBFbmNvZGluZ1BhcmFtZXRlcnMgaXMgYW4gZXhwZXJpbWVudGFsIGZlYXR1cmUuIFRoaXMgcGFyYW1ldGVyIGlzIHN1YmplY3QgdG8gY2hhbmdlLlxuICAgKiBAcmV0dXJuIHtQcm9taXNlPFB1YmxpY2F0aW9uLCBFcnJvcj59IFJldHVybmVkIHByb21pc2Ugd2lsbCBiZSByZXNvbHZlZCB3aXRoIGEgbmV3bHkgY3JlYXRlZCBQdWJsaWNhdGlvbiBvbmNlIHNwZWNpZmljIHN0cmVhbSBpcyBzdWNjZXNzZnVsbHkgcHVibGlzaGVkLCBvciByZWplY3RlZCB3aXRoIGEgbmV3bHkgY3JlYXRlZCBFcnJvciBpZiBzdHJlYW0gaXMgaW52YWxpZCBvciBvcHRpb25zIGNhbm5vdCBiZSBzYXRpc2ZpZWQuIFN1Y2Nlc3NmdWxseSBwdWJsaXNoZWQgbWVhbnMgUGVlckNvbm5lY3Rpb24gaXMgZXN0YWJsaXNoZWQgYW5kIHNlcnZlciBpcyBhYmxlIHRvIHByb2Nlc3MgbWVkaWEgZGF0YS5cbiAgICovXG4gIHRoaXMucHVibGlzaCA9IGZ1bmN0aW9uKHN0cmVhbSwgb3B0aW9ucywgdmlkZW9Db2RlY3MpIHtcbiAgICBpZiAoIShzdHJlYW0gaW5zdGFuY2VvZiBTdHJlYW1Nb2R1bGUuTG9jYWxTdHJlYW0pKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IENvbmZlcmVuY2VFcnJvcignSW52YWxpZCBzdHJlYW0uJykpO1xuICAgIH1cbiAgICBpZiAoc3RyZWFtLnNvdXJjZS5kYXRhKSB7XG4gICAgICByZXR1cm4gcXVpY1RyYW5zcG9ydENoYW5uZWwucHVibGlzaChzdHJlYW0pO1xuICAgIH1cbiAgICBpZiAocHVibGlzaENoYW5uZWxzLmhhcyhzdHJlYW0ubWVkaWFTdHJlYW0uaWQpKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IENvbmZlcmVuY2VFcnJvcihcbiAgICAgICAgICAnQ2Fubm90IHB1Ymxpc2ggYSBwdWJsaXNoZWQgc3RyZWFtLicpKTtcbiAgICB9XG4gICAgcmV0dXJuIHBlZXJDb25uZWN0aW9uQ2hhbm5lbC5wdWJsaXNoKHN0cmVhbSwgb3B0aW9ucywgdmlkZW9Db2RlY3MpO1xuICB9O1xuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gc3Vic2NyaWJlXG4gICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5Db25mZXJlbmNlQ2xpZW50XG4gICAqIEBpbnN0YW5jZVxuICAgKiBAZGVzYyBTdWJzY3JpYmUgYSBSZW1vdGVTdHJlYW0gZnJvbSBjb25mZXJlbmNlIHNlcnZlci5cbiAgICogQHBhcmFtIHtPd3QuQmFzZS5SZW1vdGVTdHJlYW19IHN0cmVhbSBUaGUgc3RyZWFtIHRvIGJlIHN1YnNjcmliZWQuXG4gICAqIEBwYXJhbSB7T3d0LkNvbmZlcmVuY2UuU3Vic2NyaWJlT3B0aW9uc30gb3B0aW9ucyBPcHRpb25zIGZvciBzdWJzY3JpcHRpb24uXG4gICAqIEByZXR1cm4ge1Byb21pc2U8U3Vic2NyaXB0aW9uLCBFcnJvcj59IFJldHVybmVkIHByb21pc2Ugd2lsbCBiZSByZXNvbHZlZCB3aXRoIGEgbmV3bHkgY3JlYXRlZCBTdWJzY3JpcHRpb24gb25jZSBzcGVjaWZpYyBzdHJlYW0gaXMgc3VjY2Vzc2Z1bGx5IHN1YnNjcmliZWQsIG9yIHJlamVjdGVkIHdpdGggYSBuZXdseSBjcmVhdGVkIEVycm9yIGlmIHN0cmVhbSBpcyBpbnZhbGlkIG9yIG9wdGlvbnMgY2Fubm90IGJlIHNhdGlzZmllZC4gU3VjY2Vzc2Z1bGx5IHN1YnNjcmliZWQgbWVhbnMgUGVlckNvbm5lY3Rpb24gaXMgZXN0YWJsaXNoZWQgYW5kIHNlcnZlciB3YXMgc3RhcnRlZCB0byBzZW5kIG1lZGlhIGRhdGEuXG4gICAqL1xuICB0aGlzLnN1YnNjcmliZSA9IGZ1bmN0aW9uKHN0cmVhbSwgb3B0aW9ucykge1xuICAgIGlmICghKHN0cmVhbSBpbnN0YW5jZW9mIFN0cmVhbU1vZHVsZS5SZW1vdGVTdHJlYW0pKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IENvbmZlcmVuY2VFcnJvcignSW52YWxpZCBzdHJlYW0uJykpO1xuICAgIH1cbiAgICBpZiAoc3RyZWFtLnNvdXJjZS5kYXRhKSB7XG4gICAgICBpZiAoc3RyZWFtLnNvdXJjZS5hdWRpbyB8fCBzdHJlYW0uc291cmNlLnZpZGVvKSB7XG4gICAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgVHlwZUVycm9yKFxuICAgICAgICAgICAgJ0ludmFsaWQgc291cmNlIGluZm8uIEEgcmVtb3RlIHN0cmVhbSBpcyBlaXRoZXIgYSBkYXRhIHN0cmVhbSBvciAnICtcbiAgICAgICAgICAgICdhIG1lZGlhIHN0cmVhbS4nKSk7XG4gICAgICB9XG4gICAgICBpZiAocXVpY1RyYW5zcG9ydENoYW5uZWwpIHtcbiAgICAgICAgcmV0dXJuIHF1aWNUcmFuc3BvcnRDaGFubmVsLnN1YnNjcmliZShzdHJlYW0pO1xuICAgICAgfSBlbHNlIHtcbiAgICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KG5ldyBUeXBlRXJyb3IoJ1dlYlRyYW5zcG9ydCBpcyBub3Qgc3VwcG9ydGVkLicpKTtcbiAgICAgIH1cbiAgICB9XG4gICAgcmV0dXJuIHBlZXJDb25uZWN0aW9uQ2hhbm5lbC5zdWJzY3JpYmUoc3RyZWFtLCBvcHRpb25zKTtcbiAgfTtcblxuICAvKipcbiAgICogQGZ1bmN0aW9uIHNlbmRcbiAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLkNvbmZlcmVuY2VDbGllbnRcbiAgICogQGluc3RhbmNlXG4gICAqIEBkZXNjIFNlbmQgYSB0ZXh0IG1lc3NhZ2UgdG8gYSBwYXJ0aWNpcGFudCBvciBhbGwgcGFydGljaXBhbnRzLlxuICAgKiBAcGFyYW0ge3N0cmluZ30gbWVzc2FnZSBNZXNzYWdlIHRvIGJlIHNlbnQuXG4gICAqIEBwYXJhbSB7c3RyaW5nfSBwYXJ0aWNpcGFudElkIFJlY2VpdmVyIG9mIHRoaXMgbWVzc2FnZS4gTWVzc2FnZSB3aWxsIGJlIHNlbnQgdG8gYWxsIHBhcnRpY2lwYW50cyBpZiBwYXJ0aWNpcGFudElkIGlzIHVuZGVmaW5lZC5cbiAgICogQHJldHVybiB7UHJvbWlzZTx2b2lkLCBFcnJvcj59IFJldHVybmVkIHByb21pc2Ugd2lsbCBiZSByZXNvbHZlZCB3aGVuIGNvbmZlcmVuY2Ugc2VydmVyIHJlY2VpdmVkIGNlcnRhaW4gbWVzc2FnZS5cbiAgICovXG4gIHRoaXMuc2VuZCA9IGZ1bmN0aW9uKG1lc3NhZ2UsIHBhcnRpY2lwYW50SWQpIHtcbiAgICBpZiAocGFydGljaXBhbnRJZCA9PT0gdW5kZWZpbmVkKSB7XG4gICAgICBwYXJ0aWNpcGFudElkID0gJ2FsbCc7XG4gICAgfVxuICAgIHJldHVybiBzZW5kU2lnbmFsaW5nTWVzc2FnZSgndGV4dCcsIHt0bzogcGFydGljaXBhbnRJZCwgbWVzc2FnZTogbWVzc2FnZX0pO1xuICB9O1xuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gbGVhdmVcbiAgICogQG1lbWJlck9mIE93dC5Db25mZXJlbmNlLkNvbmZlcmVuY2VDbGllbnRcbiAgICogQGluc3RhbmNlXG4gICAqIEBkZXNjIExlYXZlIGEgY29uZmVyZW5jZS5cbiAgICogQHJldHVybiB7UHJvbWlzZTx2b2lkLCBFcnJvcj59IFJldHVybmVkIHByb21pc2Ugd2lsbCBiZSByZXNvbHZlZCB3aXRoIHVuZGVmaW5lZCBvbmNlIHRoZSBjb25uZWN0aW9uIGlzIGRpc2Nvbm5lY3RlZC5cbiAgICovXG4gIHRoaXMubGVhdmUgPSBmdW5jdGlvbigpIHtcbiAgICByZXR1cm4gc2lnbmFsaW5nLmRpc2Nvbm5lY3QoKS50aGVuKCgpID0+IHtcbiAgICAgIGNsZWFuKCk7XG4gICAgICBzaWduYWxpbmdTdGF0ZSA9IFNpZ25hbGluZ1N0YXRlLlJFQURZO1xuICAgIH0pO1xuICB9O1xuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gY3JlYXRlU2VuZFN0cmVhbVxuICAgKiBAbWVtYmVyT2YgT3d0LkNvbmZlcmVuY2UuQ29uZmVyZW5jZUNsaWVudFxuICAgKiBAaW5zdGFuY2VcbiAgICogQGRlc2MgQ3JlYXRlIGFuIG91dGdvaW5nIHN0cmVhbS4gT25seSBhdmFpbGFibGUgd2hlbiBXZWJUcmFuc3BvcnQgaXMgc3VwcG9ydGVkIGJ5IHVzZXIncyBicm93c2VyLlxuICAgKiBAcmV0dXJuIHtQcm9taXNlPFNlbmRTdHJlYW0sIEVycm9yPn0gUmV0dXJuZWQgcHJvbWlzZSB3aWxsIGJlIHJlc29sdmVkIHdpdGggYSBTZW5kU3RyZWFtIG9uY2Ugc3RyZWFtIGlzIGNyZWF0ZWQuXG4gICAqL1xuICBpZiAoUXVpY0Nvbm5lY3Rpb24pIHtcbiAgICB0aGlzLmNyZWF0ZVNlbmRTdHJlYW0gPSBhc3luYyBmdW5jdGlvbigpIHtcbiAgICAgIGlmICghcXVpY1RyYW5zcG9ydENoYW5uZWwpIHtcbiAgICAgICAgLy8gVHJ5IHRvIGNyZWF0ZSBhIG5ldyBvbmUgb3IgY29uc2lkZXIgaXQgYXMgY2xvc2VkP1xuICAgICAgICB0aHJvdyBuZXcgQ29uZmVyZW5jZUVycm9yKCdObyBRVUlDIGNvbm5lY3Rpb24gYXZhaWxhYmxlLicpO1xuICAgICAgfVxuICAgICAgcmV0dXJuIHF1aWNUcmFuc3BvcnRDaGFubmVsLmNyZWF0ZVNlbmRTdHJlYW0oKTtcbiAgICB9O1xuICB9XG59O1xuIiwiLy8gQ29weXJpZ2h0IChDKSA8MjAxOD4gSW50ZWwgQ29ycG9yYXRpb25cbi8vXG4vLyBTUERYLUxpY2Vuc2UtSWRlbnRpZmllcjogQXBhY2hlLTIuMFxuXG4ndXNlIHN0cmljdCc7XG5cbi8qKlxuICogQGNsYXNzIENvbmZlcmVuY2VFcnJvclxuICogQGNsYXNzRGVzYyBUaGUgQ29uZmVyZW5jZUVycm9yIG9iamVjdCByZXByZXNlbnRzIGFuIGVycm9yIGluIGNvbmZlcmVuY2UgbW9kZS5cbiAqIEBtZW1iZXJPZiBPd3QuQ29uZmVyZW5jZVxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY2xhc3MgQ29uZmVyZW5jZUVycm9yIGV4dGVuZHMgRXJyb3Ige1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3RvcihtZXNzYWdlKSB7XG4gICAgc3VwZXIobWVzc2FnZSk7XG4gIH1cbn1cbiIsIi8vIENvcHlyaWdodCAoQykgPDIwMTg+IEludGVsIENvcnBvcmF0aW9uXG4vL1xuLy8gU1BEWC1MaWNlbnNlLUlkZW50aWZpZXI6IEFwYWNoZS0yLjBcblxuJ3VzZSBzdHJpY3QnO1xuXG5leHBvcnQge0NvbmZlcmVuY2VDbGllbnR9IGZyb20gJy4vY2xpZW50LmpzJztcbmV4cG9ydCB7U2lvU2lnbmFsaW5nfSBmcm9tICcuL3NpZ25hbGluZy5qcyc7XG4iLCIvLyBDb3B5cmlnaHQgKEMpIDwyMDE4PiBJbnRlbCBDb3Jwb3JhdGlvblxuLy9cbi8vIFNQRFgtTGljZW5zZS1JZGVudGlmaWVyOiBBcGFjaGUtMi4wXG5cbid1c2Ugc3RyaWN0JztcblxuLyoqXG4gKiBAY2xhc3MgQ29uZmVyZW5jZUluZm9cbiAqIEBjbGFzc0Rlc2MgSW5mb3JtYXRpb24gZm9yIGEgY29uZmVyZW5jZS5cbiAqIEBtZW1iZXJPZiBPd3QuQ29uZmVyZW5jZVxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY2xhc3MgQ29uZmVyZW5jZUluZm8ge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3RvcihpZCwgcGFydGljaXBhbnRzLCByZW1vdGVTdHJlYW1zLCBteUluZm8pIHtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHtzdHJpbmd9IGlkXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLkNvbmZlcmVuY2VJbmZvXG4gICAgICogQGRlc2MgQ29uZmVyZW5jZSBJRC5cbiAgICAgKi9cbiAgICB0aGlzLmlkID0gaWQ7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7QXJyYXk8T3d0LkNvbmZlcmVuY2UuUGFydGljaXBhbnQ+fSBwYXJ0aWNpcGFudHNcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2UuQ29uZmVyZW5jZUluZm9cbiAgICAgKiBAZGVzYyBQYXJ0aWNpcGFudHMgaW4gdGhlIGNvbmZlcmVuY2UuXG4gICAgICovXG4gICAgdGhpcy5wYXJ0aWNpcGFudHMgPSBwYXJ0aWNpcGFudHM7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7QXJyYXk8T3d0LkJhc2UuUmVtb3RlU3RyZWFtPn0gcmVtb3RlU3RyZWFtc1xuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5Db25mZXJlbmNlSW5mb1xuICAgICAqIEBkZXNjIFN0cmVhbXMgcHVibGlzaGVkIGJ5IHBhcnRpY2lwYW50cy4gSXQgYWxzbyBpbmNsdWRlcyBzdHJlYW1zIHB1Ymxpc2hlZCBieSBjdXJyZW50IHVzZXIuXG4gICAgICovXG4gICAgdGhpcy5yZW1vdGVTdHJlYW1zID0gcmVtb3RlU3RyZWFtcztcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHtPd3QuQmFzZS5QYXJ0aWNpcGFudH0gc2VsZlxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5Db25mZXJlbmNlSW5mb1xuICAgICAqL1xuICAgIHRoaXMuc2VsZiA9IG15SW5mbztcbiAgfVxufVxuIiwiLy8gQ29weXJpZ2h0IChDKSA8MjAxOD4gSW50ZWwgQ29ycG9yYXRpb25cbi8vXG4vLyBTUERYLUxpY2Vuc2UtSWRlbnRpZmllcjogQXBhY2hlLTIuMFxuXG4ndXNlIHN0cmljdCc7XG5cbmltcG9ydCAqIGFzIFN0cmVhbU1vZHVsZSBmcm9tICcuLi9iYXNlL3N0cmVhbS5qcyc7XG5pbXBvcnQgKiBhcyBTdHJlYW1VdGlsc01vZHVsZSBmcm9tICcuL3N0cmVhbXV0aWxzLmpzJztcbmltcG9ydCB7T3d0RXZlbnR9IGZyb20gJy4uL2Jhc2UvZXZlbnQuanMnO1xuXG4vKipcbiAqIEBjbGFzcyBSZW1vdGVNaXhlZFN0cmVhbVxuICogQGNsYXNzRGVzYyBNaXhlZCBzdHJlYW0gZnJvbSBjb25mZXJlbmNlIHNlcnZlci5cbiAqIEV2ZW50czpcbiAqXG4gKiB8IEV2ZW50IE5hbWUgICAgICAgICAgICAgfCBBcmd1bWVudCBUeXBlICAgIHwgRmlyZWQgd2hlbiAgICAgICB8XG4gKiB8IC0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tfCAtLS0tLS0tLS0tLS0tLS0tIHwgLS0tLS0tLS0tLS0tLS0tLSB8XG4gKiB8IGFjdGl2ZWF1ZGlvaW5wdXRjaGFuZ2UgfCBFdmVudCAgICAgICAgICAgIHwgQXVkaW8gYWN0aXZlbmVzcyBvZiBpbnB1dCBzdHJlYW0gKG9mIHRoZSBtaXhlZCBzdHJlYW0pIGlzIGNoYW5nZWQuIHxcbiAqIHwgbGF5b3V0Y2hhbmdlICAgICAgICAgICB8IEV2ZW50ICAgICAgICAgICAgfCBWaWRlbydzIGxheW91dCBoYXMgYmVlbiBjaGFuZ2VkLiBJdCB1c3VhbGx5IGhhcHBlbnMgd2hlbiBhIG5ldyB2aWRlbyBpcyBtaXhlZCBpbnRvIHRoZSB0YXJnZXQgbWl4ZWQgc3RyZWFtIG9yIGFuIGV4aXN0aW5nIHZpZGVvIGhhcyBiZWVuIHJlbW92ZWQgZnJvbSBtaXhlZCBzdHJlYW0uIHxcbiAqXG4gKiBAbWVtYmVyT2YgT3d0LkNvbmZlcmVuY2VcbiAqIEBleHRlbmRzIE93dC5CYXNlLlJlbW90ZVN0cmVhbVxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY2xhc3MgUmVtb3RlTWl4ZWRTdHJlYW0gZXh0ZW5kcyBTdHJlYW1Nb2R1bGUuUmVtb3RlU3RyZWFtIHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IoaW5mbykge1xuICAgIGlmIChpbmZvLnR5cGUgIT09ICdtaXhlZCcpIHtcbiAgICAgIHRocm93IG5ldyBUeXBlRXJyb3IoJ05vdCBhIG1peGVkIHN0cmVhbScpO1xuICAgIH1cbiAgICBzdXBlcihpbmZvLmlkLCB1bmRlZmluZWQsIHVuZGVmaW5lZCwgbmV3IFN0cmVhbU1vZHVsZS5TdHJlYW1Tb3VyY2VJbmZvKFxuICAgICAgICAnbWl4ZWQnLCAnbWl4ZWQnKSk7XG5cbiAgICB0aGlzLnNldHRpbmdzID0gU3RyZWFtVXRpbHNNb2R1bGUuY29udmVydFRvUHVibGljYXRpb25TZXR0aW5ncyhpbmZvLm1lZGlhKTtcblxuICAgIHRoaXMuZXh0cmFDYXBhYmlsaXRpZXMgPVxuICAgICAgICBTdHJlYW1VdGlsc01vZHVsZS5jb252ZXJ0VG9TdWJzY3JpcHRpb25DYXBhYmlsaXRpZXMoaW5mby5tZWRpYSk7XG4gIH1cbn1cblxuLyoqXG4gKiBAY2xhc3MgQWN0aXZlQXVkaW9JbnB1dENoYW5nZUV2ZW50XG4gKiBAY2xhc3NEZXNjIENsYXNzIEFjdGl2ZUF1ZGlvSW5wdXRDaGFuZ2VFdmVudCByZXByZXNlbnRzIGFuIGFjdGl2ZSBhdWRpbyBpbnB1dCBjaGFuZ2UgZXZlbnQuXG4gKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2VcbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNsYXNzIEFjdGl2ZUF1ZGlvSW5wdXRDaGFuZ2VFdmVudCBleHRlbmRzIE93dEV2ZW50IHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IodHlwZSwgaW5pdCkge1xuICAgIHN1cGVyKHR5cGUpO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge3N0cmluZ30gYWN0aXZlQXVkaW9JbnB1dFN0cmVhbUlkXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLkFjdGl2ZUF1ZGlvSW5wdXRDaGFuZ2VFdmVudFxuICAgICAqIEBkZXNjIFRoZSBJRCBvZiBpbnB1dCBzdHJlYW0ob2YgdGhlIG1peGVkIHN0cmVhbSkgd2hvc2UgYXVkaW8gaXMgYWN0aXZlLlxuICAgICAqL1xuICAgIHRoaXMuYWN0aXZlQXVkaW9JbnB1dFN0cmVhbUlkID0gaW5pdC5hY3RpdmVBdWRpb0lucHV0U3RyZWFtSWQ7XG4gIH1cbn1cblxuLyoqXG4gKiBAY2xhc3MgTGF5b3V0Q2hhbmdlRXZlbnRcbiAqIEBjbGFzc0Rlc2MgQ2xhc3MgTGF5b3V0Q2hhbmdlRXZlbnQgcmVwcmVzZW50cyBhbiB2aWRlbyBsYXlvdXQgY2hhbmdlIGV2ZW50LlxuICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKi9cbmV4cG9ydCBjbGFzcyBMYXlvdXRDaGFuZ2VFdmVudCBleHRlbmRzIE93dEV2ZW50IHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IodHlwZSwgaW5pdCkge1xuICAgIHN1cGVyKHR5cGUpO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge29iamVjdH0gbGF5b3V0XG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLkxheW91dENoYW5nZUV2ZW50XG4gICAgICogQGRlc2MgQ3VycmVudCB2aWRlbydzIGxheW91dC4gSXQncyBhbiBhcnJheSBvZiBtYXAgd2hpY2ggbWFwcyBlYWNoIHN0cmVhbSB0byBhIHJlZ2lvbi5cbiAgICAgKi9cbiAgICB0aGlzLmxheW91dCA9IGluaXQubGF5b3V0O1xuICB9XG59XG5cbiIsIi8vIENvcHlyaWdodCAoQykgPDIwMTg+IEludGVsIENvcnBvcmF0aW9uXG4vL1xuLy8gU1BEWC1MaWNlbnNlLUlkZW50aWZpZXI6IEFwYWNoZS0yLjBcblxuaW1wb3J0ICogYXMgRXZlbnRNb2R1bGUgZnJvbSAnLi4vYmFzZS9ldmVudC5qcyc7XG5cbid1c2Ugc3RyaWN0JztcblxuLyoqXG4gKiBAY2xhc3MgUGFydGljaXBhbnRcbiAqIEBtZW1iZXJPZiBPd3QuQ29uZmVyZW5jZVxuICogQGNsYXNzRGVzYyBUaGUgUGFydGljaXBhbnQgZGVmaW5lcyBhIHBhcnRpY2lwYW50IGluIGEgY29uZmVyZW5jZS5cbiAqIEV2ZW50czpcbiAqXG4gKiB8IEV2ZW50IE5hbWUgICAgICB8IEFyZ3VtZW50IFR5cGUgICAgICB8IEZpcmVkIHdoZW4gICAgICAgfFxuICogfCAtLS0tLS0tLS0tLS0tLS0tfCAtLS0tLS0tLS0tLS0tLS0tLS0gfCAtLS0tLS0tLS0tLS0tLS0tIHxcbiAqIHwgbGVmdCAgICAgICAgICAgIHwgT3d0LkJhc2UuT3d0RXZlbnQgIHwgVGhlIHBhcnRpY2lwYW50IGxlZnQgdGhlIGNvbmZlcmVuY2UuIHxcbiAqXG4gKiBAZXh0ZW5kcyBPd3QuQmFzZS5FdmVudERpc3BhdGNoZXJcbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNsYXNzIFBhcnRpY2lwYW50IGV4dGVuZHMgRXZlbnRNb2R1bGUuRXZlbnREaXNwYXRjaGVyIHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IoaWQsIHJvbGUsIHVzZXJJZCkge1xuICAgIHN1cGVyKCk7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7c3RyaW5nfSBpZFxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5QYXJ0aWNpcGFudFxuICAgICAqIEBkZXNjIFRoZSBJRCBvZiB0aGUgcGFydGljaXBhbnQuIEl0IHZhcmllcyB3aGVuIGEgc2luZ2xlIHVzZXIgam9pbiBkaWZmZXJlbnQgY29uZmVyZW5jZXMuXG4gICAgICovXG4gICAgT2JqZWN0LmRlZmluZVByb3BlcnR5KHRoaXMsICdpZCcsIHtcbiAgICAgIGNvbmZpZ3VyYWJsZTogZmFsc2UsXG4gICAgICB3cml0YWJsZTogZmFsc2UsXG4gICAgICB2YWx1ZTogaWQsXG4gICAgfSk7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7c3RyaW5nfSByb2xlXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlBhcnRpY2lwYW50XG4gICAgICovXG4gICAgT2JqZWN0LmRlZmluZVByb3BlcnR5KHRoaXMsICdyb2xlJywge1xuICAgICAgY29uZmlndXJhYmxlOiBmYWxzZSxcbiAgICAgIHdyaXRhYmxlOiBmYWxzZSxcbiAgICAgIHZhbHVlOiByb2xlLFxuICAgIH0pO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge3N0cmluZ30gdXNlcklkXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlBhcnRpY2lwYW50XG4gICAgICogQGRlc2MgVGhlIHVzZXIgSUQgb2YgdGhlIHBhcnRpY2lwYW50LiBJdCBjYW4gYmUgaW50ZWdyYXRlZCBpbnRvIGV4aXN0aW5nIGFjY291bnQgbWFuYWdlbWVudCBzeXN0ZW0uXG4gICAgICovXG4gICAgT2JqZWN0LmRlZmluZVByb3BlcnR5KHRoaXMsICd1c2VySWQnLCB7XG4gICAgICBjb25maWd1cmFibGU6IGZhbHNlLFxuICAgICAgd3JpdGFibGU6IGZhbHNlLFxuICAgICAgdmFsdWU6IHVzZXJJZCxcbiAgICB9KTtcbiAgfVxufVxuIiwiLy8gQ29weXJpZ2h0IChDKSA8MjAxOD4gSW50ZWwgQ29ycG9yYXRpb25cbi8vXG4vLyBTUERYLUxpY2Vuc2UtSWRlbnRpZmllcjogQXBhY2hlLTIuMFxuXG4vKiBlc2xpbnQtZGlzYWJsZSByZXF1aXJlLWpzZG9jICovXG4vKiBnbG9iYWwgUHJvbWlzZSwgTWFwLCBXZWJUcmFuc3BvcnQsIFVpbnQ4QXJyYXksIFVpbnQzMkFycmF5LCBUZXh0RW5jb2RlciAqL1xuXG4ndXNlIHN0cmljdCc7XG5cbmltcG9ydCBMb2dnZXIgZnJvbSAnLi4vYmFzZS9sb2dnZXIuanMnO1xuaW1wb3J0IHtFdmVudERpc3BhdGNoZXJ9IGZyb20gJy4uL2Jhc2UvZXZlbnQuanMnO1xuaW1wb3J0IHtQdWJsaWNhdGlvbn0gZnJvbSAnLi4vYmFzZS9wdWJsaWNhdGlvbi5qcyc7XG5pbXBvcnQge1N1YnNjcmlwdGlvbn0gZnJvbSAnLi9zdWJzY3JpcHRpb24uanMnO1xuaW1wb3J0IHtCYXNlNjR9IGZyb20gJy4uL2Jhc2UvYmFzZTY0LmpzJztcblxuLyoqXG4gKiBAY2xhc3MgUXVpY0Nvbm5lY3Rpb25cbiAqIEBjbGFzc0Rlc2MgQSBjaGFubmVsIGZvciBhIFFVSUMgdHJhbnNwb3J0IGJldHdlZW4gY2xpZW50IGFuZCBjb25mZXJlbmNlXG4gKiBzZXJ2ZXIuXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKiBAcHJpdmF0ZVxuICovXG5leHBvcnQgY2xhc3MgUXVpY0Nvbm5lY3Rpb24gZXh0ZW5kcyBFdmVudERpc3BhdGNoZXIge1xuICAvLyBgdG9rZW5TdHJpbmdgIGlzIGEgYmFzZTY0IHN0cmluZyBvZiB0aGUgdG9rZW4gb2JqZWN0LiBJdCdzIGluIHRoZSByZXR1cm5cbiAgLy8gdmFsdWUgb2YgYENvbmZlcmVuY2VDbGllbnQuam9pbmAuXG4gIGNvbnN0cnVjdG9yKHVybCwgdG9rZW5TdHJpbmcsIHNpZ25hbGluZywgd2ViVHJhbnNwb3J0T3B0aW9ucykge1xuICAgIHN1cGVyKCk7XG4gICAgdGhpcy5fdG9rZW5TdHJpbmcgPSB0b2tlblN0cmluZztcbiAgICB0aGlzLl90b2tlbiA9IEpTT04ucGFyc2UoQmFzZTY0LmRlY29kZUJhc2U2NCh0b2tlblN0cmluZykpO1xuICAgIHRoaXMuX3NpZ25hbGluZyA9IHNpZ25hbGluZztcbiAgICB0aGlzLl9lbmRlZCA9IGZhbHNlO1xuICAgIHRoaXMuX3F1aWNTdHJlYW1zID0gbmV3IE1hcCgpOyAvLyBLZXkgaXMgcHVibGljYXRpb24gb3Igc3Vic2NyaXB0aW9uIElELlxuICAgIHRoaXMuX3F1aWNUcmFuc3BvcnQgPSBuZXcgV2ViVHJhbnNwb3J0KHVybCwgd2ViVHJhbnNwb3J0T3B0aW9ucyk7XG4gICAgdGhpcy5fc3Vic2NyaWJlUHJvbWlzZXMgPSBuZXcgTWFwKCk7IC8vIEtleSBpcyBzdWJzY3JpcHRpb24gSUQuXG4gICAgdGhpcy5fdHJhbnNwb3J0SWQgPSB0aGlzLl90b2tlbi50cmFuc3BvcnRJZDtcbiAgICB0aGlzLl9pbml0UmVjZWl2ZVN0cmVhbVJlYWRlcigpO1xuICB9XG5cbiAgLyoqXG4gICAqIEBmdW5jdGlvbiBvbk1lc3NhZ2VcbiAgICogQGRlc2MgUmVjZWl2ZWQgYSBtZXNzYWdlIGZyb20gY29uZmVyZW5jZSBwb3J0YWwuIERlZmluZWQgaW4gY2xpZW50LXNlcnZlclxuICAgKiBwcm90b2NvbC5cbiAgICogQHBhcmFtIHtzdHJpbmd9IG5vdGlmaWNhdGlvbiBOb3RpZmljYXRpb24gdHlwZS5cbiAgICogQHBhcmFtIHtvYmplY3R9IG1lc3NhZ2UgTWVzc2FnZSByZWNlaXZlZC5cbiAgICogQHByaXZhdGVcbiAgICovXG4gIG9uTWVzc2FnZShub3RpZmljYXRpb24sIG1lc3NhZ2UpIHtcbiAgICBzd2l0Y2ggKG5vdGlmaWNhdGlvbikge1xuICAgICAgY2FzZSAncHJvZ3Jlc3MnOlxuICAgICAgICBpZiAobWVzc2FnZS5zdGF0dXMgPT09ICdzb2FjJykge1xuICAgICAgICAgIHRoaXMuX3NvYWNIYW5kbGVyKG1lc3NhZ2UuZGF0YSk7XG4gICAgICAgIH0gZWxzZSBpZiAobWVzc2FnZS5zdGF0dXMgPT09ICdyZWFkeScpIHtcbiAgICAgICAgICB0aGlzLl9yZWFkeUhhbmRsZXIoKTtcbiAgICAgICAgfSBlbHNlIGlmIChtZXNzYWdlLnN0YXR1cyA9PT0gJ2Vycm9yJykge1xuICAgICAgICAgIHRoaXMuX2Vycm9ySGFuZGxlcihtZXNzYWdlLmRhdGEpO1xuICAgICAgICB9XG4gICAgICAgIGJyZWFrO1xuICAgICAgY2FzZSAnc3RyZWFtJzpcbiAgICAgICAgdGhpcy5fb25TdHJlYW1FdmVudChtZXNzYWdlKTtcbiAgICAgICAgYnJlYWs7XG4gICAgICBkZWZhdWx0OlxuICAgICAgICBMb2dnZXIud2FybmluZygnVW5rbm93biBub3RpZmljYXRpb24gZnJvbSBNQ1UuJyk7XG4gICAgfVxuICB9XG5cbiAgYXN5bmMgaW5pdCgpIHtcbiAgICBhd2FpdCB0aGlzLl9hdXRoZW50aWNhdGUodGhpcy5fdG9rZW5TdHJpbmcpO1xuICB9XG5cbiAgYXN5bmMgX2luaXRSZWNlaXZlU3RyZWFtUmVhZGVyKCkge1xuICAgIGNvbnN0IHJlY2VpdmVTdHJlYW1SZWFkZXIgPVxuICAgICAgICB0aGlzLl9xdWljVHJhbnNwb3J0LmluY29taW5nQmlkaXJlY3Rpb25hbFN0cmVhbXMuZ2V0UmVhZGVyKCk7XG4gICAgTG9nZ2VyLmluZm8oJ1JlYWRlcjogJyArIHJlY2VpdmVTdHJlYW1SZWFkZXIpO1xuICAgIGxldCByZWNlaXZpbmdEb25lID0gZmFsc2U7XG4gICAgd2hpbGUgKCFyZWNlaXZpbmdEb25lKSB7XG4gICAgICBjb25zdCB7dmFsdWU6IHJlY2VpdmVTdHJlYW0sIGRvbmU6IHJlYWRpbmdSZWNlaXZlU3RyZWFtc0RvbmV9ID1cbiAgICAgICAgICBhd2FpdCByZWNlaXZlU3RyZWFtUmVhZGVyLnJlYWQoKTtcbiAgICAgIExvZ2dlci5pbmZvKCdOZXcgc3RyZWFtIHJlY2VpdmVkJyk7XG4gICAgICBpZiAocmVhZGluZ1JlY2VpdmVTdHJlYW1zRG9uZSkge1xuICAgICAgICByZWNlaXZpbmdEb25lID0gdHJ1ZTtcbiAgICAgICAgYnJlYWs7XG4gICAgICB9XG4gICAgICBjb25zdCBjaHVua1JlYWRlciA9IHJlY2VpdmVTdHJlYW0ucmVhZGFibGUuZ2V0UmVhZGVyKCk7XG4gICAgICBjb25zdCB7dmFsdWU6IHV1aWQsIGRvbmU6IHJlYWRpbmdDaHVua3NEb25lfSA9IGF3YWl0IGNodW5rUmVhZGVyLnJlYWQoKTtcbiAgICAgIGlmIChyZWFkaW5nQ2h1bmtzRG9uZSkge1xuICAgICAgICBMb2dnZXIuZXJyb3IoJ1N0cmVhbSBjbG9zZWQgdW5leHBlY3RlZGx5LicpO1xuICAgICAgICByZXR1cm47XG4gICAgICB9XG4gICAgICBpZiAodXVpZC5sZW5ndGggIT0gMTYpIHtcbiAgICAgICAgTG9nZ2VyLmVycm9yKCdVbmV4cGVjdGVkIGxlbmd0aCBmb3IgVVVJRC4nKTtcbiAgICAgICAgcmV0dXJuO1xuICAgICAgfVxuICAgICAgY2h1bmtSZWFkZXIucmVsZWFzZUxvY2soKTtcbiAgICAgIGNvbnN0IHN1YnNjcmlwdGlvbklkID0gdGhpcy5fdWludDhBcnJheVRvVXVpZCh1dWlkKTtcbiAgICAgIHRoaXMuX3F1aWNTdHJlYW1zLnNldChzdWJzY3JpcHRpb25JZCwgcmVjZWl2ZVN0cmVhbSk7XG4gICAgICBpZiAodGhpcy5fc3Vic2NyaWJlUHJvbWlzZXMuaGFzKHN1YnNjcmlwdGlvbklkKSkge1xuICAgICAgICBjb25zdCBzdWJzY3JpcHRpb24gPVxuICAgICAgICAgICAgdGhpcy5fY3JlYXRlU3Vic2NyaXB0aW9uKHN1YnNjcmlwdGlvbklkLCByZWNlaXZlU3RyZWFtKTtcbiAgICAgICAgdGhpcy5fc3Vic2NyaWJlUHJvbWlzZXMuZ2V0KHN1YnNjcmlwdGlvbklkKS5yZXNvbHZlKHN1YnNjcmlwdGlvbik7XG4gICAgICB9XG4gICAgfVxuICB9XG5cbiAgX2NyZWF0ZVN1YnNjcmlwdGlvbihpZCwgcmVjZWl2ZVN0cmVhbSkge1xuICAgIC8vIFRPRE86IEluY29tcGxldGUgc3Vic2NyaXB0aW9uLlxuICAgIGNvbnN0IHN1YnNjcmlwdGlvbiA9IG5ldyBTdWJzY3JpcHRpb24oaWQsICgpID0+IHtcbiAgICAgIHJlY2VpdmVTdHJlYW0uYWJvcnRSZWFkaW5nKCk7XG4gICAgfSk7XG4gICAgc3Vic2NyaXB0aW9uLnN0cmVhbSA9IHJlY2VpdmVTdHJlYW07XG4gICAgcmV0dXJuIHN1YnNjcmlwdGlvbjtcbiAgfVxuXG4gIGFzeW5jIF9hdXRoZW50aWNhdGUodG9rZW4pIHtcbiAgICBhd2FpdCB0aGlzLl9xdWljVHJhbnNwb3J0LnJlYWR5O1xuICAgIGNvbnN0IHF1aWNTdHJlYW0gPSBhd2FpdCB0aGlzLl9xdWljVHJhbnNwb3J0LmNyZWF0ZUJpZGlyZWN0aW9uYWxTdHJlYW0oKTtcbiAgICBjb25zdCBjaHVua1JlYWRlciA9IHF1aWNTdHJlYW0ucmVhZGFibGUuZ2V0UmVhZGVyKCk7XG4gICAgY29uc3Qgd3JpdGVyID0gcXVpY1N0cmVhbS53cml0YWJsZS5nZXRXcml0ZXIoKTtcbiAgICBhd2FpdCB3cml0ZXIucmVhZHk7XG4gICAgLy8gMTI4IGJpdCBvZiB6ZXJvIGluZGljYXRlcyB0aGlzIGlzIGEgc3RyZWFtIGZvciBzaWduYWxpbmcuXG4gICAgd3JpdGVyLndyaXRlKG5ldyBVaW50OEFycmF5KDE2KSk7XG4gICAgLy8gU2VuZCB0b2tlbiBhcyBkZXNjcmliZWQgaW5cbiAgICAvLyBodHRwczovL2dpdGh1Yi5jb20vb3Blbi13ZWJydGMtdG9vbGtpdC9vd3Qtc2VydmVyL2Jsb2IvMjBlOGFhZDIxNmNjNDQ2MDk1ZjYzYzQwOTMzOWMzNGM3ZWU3NzBlZS9kb2MvZGVzaWduL3F1aWMtdHJhbnNwb3J0LXBheWxvYWQtZm9ybWF0Lm1kLlxuICAgIGNvbnN0IGVuY29kZXIgPSBuZXcgVGV4dEVuY29kZXIoKTtcbiAgICBjb25zdCBlbmNvZGVkVG9rZW4gPSBlbmNvZGVyLmVuY29kZSh0b2tlbik7XG4gICAgd3JpdGVyLndyaXRlKFVpbnQzMkFycmF5Lm9mKGVuY29kZWRUb2tlbi5sZW5ndGgpKTtcbiAgICB3cml0ZXIud3JpdGUoZW5jb2RlZFRva2VuKTtcbiAgICAvLyBTZXJ2ZXIgcmV0dXJucyB0cmFuc3BvcnQgSUQgYXMgYW4gYWNrLiBJZ25vcmUgaXQgaGVyZS5cbiAgICBhd2FpdCBjaHVua1JlYWRlci5yZWFkKCk7XG4gICAgTG9nZ2VyLmluZm8oJ0F1dGhlbnRpY2F0aW9uIHN1Y2Nlc3MuJyk7XG4gIH1cblxuICBhc3luYyBjcmVhdGVTZW5kU3RyZWFtKCkge1xuICAgIGF3YWl0IHRoaXMuX3F1aWNUcmFuc3BvcnQucmVhZHk7XG4gICAgY29uc3QgcXVpY1N0cmVhbSA9IGF3YWl0IHRoaXMuX3F1aWNUcmFuc3BvcnQuY3JlYXRlQmlkaXJlY3Rpb25hbFN0cmVhbSgpO1xuICAgIHJldHVybiBxdWljU3RyZWFtO1xuICB9XG5cbiAgYXN5bmMgY3JlYXRlU2VuZFN0cmVhbTEoc2Vzc2lvbklkKSB7XG4gICAgTG9nZ2VyLmluZm8oJ0NyZWF0ZSBzdHJlYW0uJyk7XG4gICAgYXdhaXQgdGhpcy5fcXVpY1RyYW5zcG9ydC5yZWFkeTtcbiAgICAvLyBUT0RPOiBQb3RlbnRpYWwgZmFpbHVyZSBiZWNhdXNlIG9mIHB1YmxpY2F0aW9uIHN0cmVhbSBpcyBjcmVhdGVkIGZhc3RlclxuICAgIC8vIHRoYW4gc2lnbmFsaW5nIHN0cmVhbShjcmVhdGVkIGJ5IHRoZSAxc3QgY2FsbCB0byBpbml0aWF0ZVB1YmxpY2F0aW9uKS5cbiAgICBjb25zdCBwdWJsaWNhdGlvbklkID0gYXdhaXQgdGhpcy5faW5pdGlhdGVQdWJsaWNhdGlvbigpO1xuICAgIGNvbnN0IHF1aWNTdHJlYW0gPSBhd2FpdCB0aGlzLl9xdWljVHJhbnNwb3J0LmNyZWF0ZVNlbmRTdHJlYW0oKTtcbiAgICBjb25zdCB3cml0ZXIgPSBxdWljU3RyZWFtLndyaXRhYmxlLmdldFdyaXRlcigpO1xuICAgIGF3YWl0IHdyaXRlci5yZWFkeTtcbiAgICB3cml0ZXIud3JpdGUodGhpcy5fdXVpZFRvVWludDhBcnJheShwdWJsaWNhdGlvbklkKSk7XG4gICAgd3JpdGVyLnJlbGVhc2VMb2NrKCk7XG4gICAgdGhpcy5fcXVpY1N0cmVhbXMuc2V0KHB1YmxpY2F0aW9uSWQsIHF1aWNTdHJlYW0pO1xuICAgIHJldHVybiBxdWljU3RyZWFtO1xuICB9XG5cbiAgYXN5bmMgcHVibGlzaChzdHJlYW0pIHtcbiAgICAvLyBUT0RPOiBBdm9pZCBhIHN0cmVhbSB0byBiZSBwdWJsaXNoZWQgdHdpY2UuIFRoZSBmaXJzdCAxNiBiaXQgZGF0YSBzZW5kIHRvXG4gICAgLy8gc2VydmVyIG11c3QgYmUgaXQncyBwdWJsaWNhdGlvbiBJRC5cbiAgICAvLyBUT0RPOiBQb3RlbnRpYWwgZmFpbHVyZSBiZWNhdXNlIG9mIHB1YmxpY2F0aW9uIHN0cmVhbSBpcyBjcmVhdGVkIGZhc3RlclxuICAgIC8vIHRoYW4gc2lnbmFsaW5nIHN0cmVhbShjcmVhdGVkIGJ5IHRoZSAxc3QgY2FsbCB0byBpbml0aWF0ZVB1YmxpY2F0aW9uKS5cbiAgICBjb25zdCBwdWJsaWNhdGlvbklkID0gYXdhaXQgdGhpcy5faW5pdGlhdGVQdWJsaWNhdGlvbigpO1xuICAgIGNvbnN0IHF1aWNTdHJlYW0gPSBzdHJlYW0uc3RyZWFtO1xuICAgIGNvbnN0IHdyaXRlciA9IHF1aWNTdHJlYW0ud3JpdGFibGUuZ2V0V3JpdGVyKCk7XG4gICAgYXdhaXQgd3JpdGVyLnJlYWR5O1xuICAgIHdyaXRlci53cml0ZSh0aGlzLl91dWlkVG9VaW50OEFycmF5KHB1YmxpY2F0aW9uSWQpKTtcbiAgICB3cml0ZXIucmVsZWFzZUxvY2soKTtcbiAgICBMb2dnZXIuaW5mbygncHVibGlzaCBpZCcpO1xuICAgIHRoaXMuX3F1aWNTdHJlYW1zLnNldChwdWJsaWNhdGlvbklkLCBxdWljU3RyZWFtKTtcbiAgICBjb25zdCBwdWJsaWNhdGlvbiA9IG5ldyBQdWJsaWNhdGlvbihwdWJsaWNhdGlvbklkLCAoKSA9PiB7XG4gICAgICB0aGlzLl9zaWduYWxpbmcuc2VuZFNpZ25hbGluZ01lc3NhZ2UoJ3VucHVibGlzaCcsIHtpZDogcHVibGljYXRpb259KVxuICAgICAgICAgIC5jYXRjaCgoZSkgPT4ge1xuICAgICAgICAgICAgTG9nZ2VyLndhcm5pbmcoJ01DVSByZXR1cm5zIG5lZ2F0aXZlIGFjayBmb3IgdW5wdWJsaXNoaW5nLCAnICsgZSk7XG4gICAgICAgICAgfSk7XG4gICAgfSAvKiBUT0RPOiBnZXRTdGF0cywgbXV0ZSwgdW5tdXRlIGlzIG5vdCBpbXBsZW1lbnRlZCAqLyk7XG4gICAgcmV0dXJuIHB1YmxpY2F0aW9uO1xuICB9XG5cbiAgaGFzQ29udGVudFNlc3Npb25JZChpZCkge1xuICAgIHJldHVybiB0aGlzLl9xdWljU3RyZWFtcy5oYXMoaWQpO1xuICB9XG5cbiAgX3V1aWRUb1VpbnQ4QXJyYXkodXVpZFN0cmluZykge1xuICAgIGlmICh1dWlkU3RyaW5nLmxlbmd0aCAhPSAzMikge1xuICAgICAgdGhyb3cgbmV3IFR5cGVFcnJvcignSW5jb3JyZWN0IFVVSUQuJyk7XG4gICAgfVxuICAgIGNvbnN0IHV1aWRBcnJheSA9IG5ldyBVaW50OEFycmF5KDE2KTtcbiAgICBmb3IgKGxldCBpID0gMDsgaSA8IDE2OyBpKyspIHtcbiAgICAgIHV1aWRBcnJheVtpXSA9IHBhcnNlSW50KHV1aWRTdHJpbmcuc3Vic3RyaW5nKGkgKiAyLCBpICogMiArIDIpLCAxNik7XG4gICAgfVxuICAgIHJldHVybiB1dWlkQXJyYXk7XG4gIH1cblxuICBfdWludDhBcnJheVRvVXVpZCh1dWlkQnl0ZXMpIHtcbiAgICBsZXQgcyA9ICcnO1xuICAgIGZvciAoY29uc3QgaGV4IG9mIHV1aWRCeXRlcykge1xuICAgICAgY29uc3Qgc3RyID0gaGV4LnRvU3RyaW5nKDE2KTtcbiAgICAgIHMgKz0gc3RyLnBhZFN0YXJ0KDIsICcwJyk7XG4gICAgfVxuICAgIHJldHVybiBzO1xuICB9XG5cbiAgc3Vic2NyaWJlKHN0cmVhbSkge1xuICAgIGNvbnN0IHAgPSBuZXcgUHJvbWlzZSgocmVzb2x2ZSwgcmVqZWN0KSA9PiB7XG4gICAgICB0aGlzLl9zaWduYWxpbmdcbiAgICAgICAgICAuc2VuZFNpZ25hbGluZ01lc3NhZ2UoJ3N1YnNjcmliZScsIHtcbiAgICAgICAgICAgIG1lZGlhOiBudWxsLFxuICAgICAgICAgICAgZGF0YToge2Zyb206IHN0cmVhbS5pZH0sXG4gICAgICAgICAgICB0cmFuc3BvcnQ6IHt0eXBlOiAncXVpYycsIGlkOiB0aGlzLl90cmFuc3BvcnRJZH0sXG4gICAgICAgICAgfSlcbiAgICAgICAgICAudGhlbigoZGF0YSkgPT4ge1xuICAgICAgICAgICAgaWYgKHRoaXMuX3F1aWNTdHJlYW1zLmhhcyhkYXRhLmlkKSkge1xuICAgICAgICAgICAgICAvLyBRVUlDIHN0cmVhbSBjcmVhdGVkIGJlZm9yZSBzaWduYWxpbmcgcmV0dXJucy5cbiAgICAgICAgICAgICAgY29uc3Qgc3Vic2NyaXB0aW9uID0gdGhpcy5fY3JlYXRlU3Vic2NyaXB0aW9uKFxuICAgICAgICAgICAgICAgICAgZGF0YS5pZCwgdGhpcy5fcXVpY1N0cmVhbXMuZ2V0KGRhdGEuaWQpKTtcbiAgICAgICAgICAgICAgcmVzb2x2ZShzdWJzY3JpcHRpb24pO1xuICAgICAgICAgICAgfSBlbHNlIHtcbiAgICAgICAgICAgICAgdGhpcy5fcXVpY1N0cmVhbXMuc2V0KGRhdGEuaWQsIG51bGwpO1xuICAgICAgICAgICAgICAvLyBRVUlDIHN0cmVhbSBpcyBub3QgY3JlYXRlZCB5ZXQsIHJlc29sdmUgcHJvbWlzZSBhZnRlciBnZXR0aW5nXG4gICAgICAgICAgICAgIC8vIFFVSUMgc3RyZWFtLlxuICAgICAgICAgICAgICB0aGlzLl9zdWJzY3JpYmVQcm9taXNlcy5zZXQoXG4gICAgICAgICAgICAgICAgICBkYXRhLmlkLCB7cmVzb2x2ZTogcmVzb2x2ZSwgcmVqZWN0OiByZWplY3R9KTtcbiAgICAgICAgICAgIH1cbiAgICAgICAgICB9KTtcbiAgICB9KTtcbiAgICByZXR1cm4gcDtcbiAgfVxuXG4gIF9yZWFkQW5kUHJpbnQoKSB7XG4gICAgdGhpcy5fcXVpY1N0cmVhbXNbMF0ud2FpdEZvclJlYWRhYmxlKDUpLnRoZW4oKCkgPT4ge1xuICAgICAgY29uc3QgZGF0YSA9IG5ldyBVaW50OEFycmF5KHRoaXMuX3F1aWNTdHJlYW1zWzBdLnJlYWRCdWZmZXJlZEFtb3VudCk7XG4gICAgICB0aGlzLl9xdWljU3RyZWFtc1swXS5yZWFkSW50byhkYXRhKTtcbiAgICAgIExvZ2dlci5pbmZvKCdSZWFkIGRhdGE6ICcgKyBkYXRhKTtcbiAgICAgIHRoaXMuX3JlYWRBbmRQcmludCgpO1xuICAgIH0pO1xuICB9XG5cbiAgYXN5bmMgX2luaXRpYXRlUHVibGljYXRpb24oKSB7XG4gICAgY29uc3QgZGF0YSA9IGF3YWl0IHRoaXMuX3NpZ25hbGluZy5zZW5kU2lnbmFsaW5nTWVzc2FnZSgncHVibGlzaCcsIHtcbiAgICAgIG1lZGlhOiBudWxsLFxuICAgICAgZGF0YTogdHJ1ZSxcbiAgICAgIHRyYW5zcG9ydDoge3R5cGU6ICdxdWljJywgaWQ6IHRoaXMuX3RyYW5zcG9ydElkfSxcbiAgICB9KTtcbiAgICBpZiAodGhpcy5fdHJhbnNwb3J0SWQgIT09IGRhdGEudHJhbnNwb3J0SWQpIHtcbiAgICAgIHRocm93IG5ldyBFcnJvcignVHJhbnNwb3J0IElEIG5vdCBtYXRjaC4nKTtcbiAgICB9XG4gICAgcmV0dXJuIGRhdGEuaWQ7XG4gIH1cblxuICBfcmVhZHlIYW5kbGVyKCkge1xuICAgIC8vIFJlYWR5IG1lc3NhZ2UgZnJvbSBzZXJ2ZXIgaXMgdXNlbGVzcyBmb3IgUXVpY1N0cmVhbSBzaW5jZSBRdWljU3RyZWFtIGhhc1xuICAgIC8vIGl0cyBvd24gc3RhdHVzLiBEbyBub3RoaW5nIGhlcmUuXG4gIH1cbn1cbiIsIi8vIENvcHlyaWdodCAoQykgPDIwMTg+IEludGVsIENvcnBvcmF0aW9uXG4vL1xuLy8gU1BEWC1MaWNlbnNlLUlkZW50aWZpZXI6IEFwYWNoZS0yLjBcblxuLyogZ2xvYmFsIGlvLCBQcm9taXNlLCBzZXRUaW1lb3V0LCBjbGVhclRpbWVvdXQgKi9cbmltcG9ydCBMb2dnZXIgZnJvbSAnLi4vYmFzZS9sb2dnZXIuanMnO1xuaW1wb3J0ICogYXMgRXZlbnRNb2R1bGUgZnJvbSAnLi4vYmFzZS9ldmVudC5qcyc7XG5pbXBvcnQge0NvbmZlcmVuY2VFcnJvcn0gZnJvbSAnLi9lcnJvci5qcyc7XG5pbXBvcnQge0Jhc2U2NH0gZnJvbSAnLi4vYmFzZS9iYXNlNjQuanMnO1xuXG4ndXNlIHN0cmljdCc7XG5cbmNvbnN0IHJlY29ubmVjdGlvbkF0dGVtcHRzID0gMTA7XG5cbi8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG5mdW5jdGlvbiBoYW5kbGVSZXNwb25zZShzdGF0dXMsIGRhdGEsIHJlc29sdmUsIHJlamVjdCkge1xuICBpZiAoc3RhdHVzID09PSAnb2snIHx8IHN0YXR1cyA9PT0gJ3N1Y2Nlc3MnKSB7XG4gICAgcmVzb2x2ZShkYXRhKTtcbiAgfSBlbHNlIGlmIChzdGF0dXMgPT09ICdlcnJvcicpIHtcbiAgICByZWplY3QoZGF0YSk7XG4gIH0gZWxzZSB7XG4gICAgTG9nZ2VyLmVycm9yKCdNQ1UgcmV0dXJucyB1bmtub3duIGFjay4nKTtcbiAgfVxufVxuXG4vKipcbiAqIEBjbGFzcyBTaW9TaWduYWxpbmdcbiAqIEBjbGFzc2Rlc2MgU29ja2V0LklPIHNpZ25hbGluZyBjaGFubmVsIGZvciBDb25mZXJlbmNlQ2xpZW50LiBJdCBpcyBub3QgcmVjb21tZW5kZWQgdG8gZGlyZWN0bHkgYWNjZXNzIHRoaXMgY2xhc3MuXG4gKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2VcbiAqIEBleHRlbmRzIE93dC5CYXNlLkV2ZW50RGlzcGF0Y2hlclxuICogQGNvbnN0cnVjdG9yXG4gKi9cbmV4cG9ydCBjbGFzcyBTaW9TaWduYWxpbmcgZXh0ZW5kcyBFdmVudE1vZHVsZS5FdmVudERpc3BhdGNoZXIge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3RvcigpIHtcbiAgICBzdXBlcigpO1xuICAgIHRoaXMuX3NvY2tldCA9IG51bGw7XG4gICAgdGhpcy5fbG9nZ2VkSW4gPSBmYWxzZTtcbiAgICB0aGlzLl9yZWNvbm5lY3RUaW1lcyA9IDA7XG4gICAgdGhpcy5fcmVjb25uZWN0aW9uVGlja2V0ID0gbnVsbDtcbiAgICB0aGlzLl9yZWZyZXNoUmVjb25uZWN0aW9uVGlja2V0ID0gbnVsbDtcbiAgfVxuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gY29ubmVjdFxuICAgKiBAaW5zdGFuY2VcbiAgICogQGRlc2MgQ29ubmVjdCB0byBhIHBvcnRhbC5cbiAgICogQG1lbWJlcm9mIE9tcy5Db25mZXJlbmNlLlNpb1NpZ25hbGluZ1xuICAgKiBAcmV0dXJuIHtQcm9taXNlPE9iamVjdCwgRXJyb3I+fSBSZXR1cm4gYSBwcm9taXNlIHJlc29sdmVkIHdpdGggdGhlIGRhdGEgcmV0dXJuZWQgYnkgcG9ydGFsIGlmIHN1Y2Nlc3NmdWxseSBsb2dnZWQgaW4uIE9yIHJldHVybiBhIHByb21pc2UgcmVqZWN0ZWQgd2l0aCBhIG5ld2x5IGNyZWF0ZWQgT21zLkVycm9yIGlmIGZhaWxlZC5cbiAgICogQHBhcmFtIHtzdHJpbmd9IGhvc3QgSG9zdCBvZiB0aGUgcG9ydGFsLlxuICAgKiBAcGFyYW0ge3N0cmluZ30gaXNTZWN1cmVkIElzIHNlY3VyZSBjb25uZWN0aW9uIG9yIG5vdC5cbiAgICogQHBhcmFtIHtzdHJpbmd9IGxvZ2luSW5mbyBJbmZvcm1hdGlvbiByZXF1aXJlZCBmb3IgbG9nZ2luZyBpbi5cbiAgICogQHByaXZhdGUuXG4gICAqL1xuICBjb25uZWN0KGhvc3QsIGlzU2VjdXJlZCwgbG9naW5JbmZvKSB7XG4gICAgcmV0dXJuIG5ldyBQcm9taXNlKChyZXNvbHZlLCByZWplY3QpID0+IHtcbiAgICAgIGNvbnN0IG9wdHMgPSB7XG4gICAgICAgICdyZWNvbm5lY3Rpb24nOiB0cnVlLFxuICAgICAgICAncmVjb25uZWN0aW9uQXR0ZW1wdHMnOiByZWNvbm5lY3Rpb25BdHRlbXB0cyxcbiAgICAgICAgJ2ZvcmNlIG5ldyBjb25uZWN0aW9uJzogdHJ1ZSxcbiAgICAgIH07XG4gICAgICB0aGlzLl9zb2NrZXQgPSBpbyhob3N0LCBvcHRzKTtcbiAgICAgIFsncGFydGljaXBhbnQnLCAndGV4dCcsICdzdHJlYW0nLCAncHJvZ3Jlc3MnXS5mb3JFYWNoKChcbiAgICAgICAgICBub3RpZmljYXRpb24pID0+IHtcbiAgICAgICAgdGhpcy5fc29ja2V0Lm9uKG5vdGlmaWNhdGlvbiwgKGRhdGEpID0+IHtcbiAgICAgICAgICB0aGlzLmRpc3BhdGNoRXZlbnQobmV3IEV2ZW50TW9kdWxlLk1lc3NhZ2VFdmVudCgnZGF0YScsIHtcbiAgICAgICAgICAgIG1lc3NhZ2U6IHtcbiAgICAgICAgICAgICAgbm90aWZpY2F0aW9uOiBub3RpZmljYXRpb24sXG4gICAgICAgICAgICAgIGRhdGE6IGRhdGEsXG4gICAgICAgICAgICB9LFxuICAgICAgICAgIH0pKTtcbiAgICAgICAgfSk7XG4gICAgICB9KTtcbiAgICAgIHRoaXMuX3NvY2tldC5vbigncmVjb25uZWN0aW5nJywgKCkgPT4ge1xuICAgICAgICB0aGlzLl9yZWNvbm5lY3RUaW1lcysrO1xuICAgICAgfSk7XG4gICAgICB0aGlzLl9zb2NrZXQub24oJ3JlY29ubmVjdF9mYWlsZWQnLCAoKSA9PiB7XG4gICAgICAgIGlmICh0aGlzLl9yZWNvbm5lY3RUaW1lcyA+PSByZWNvbm5lY3Rpb25BdHRlbXB0cykge1xuICAgICAgICAgIHRoaXMuZGlzcGF0Y2hFdmVudChuZXcgRXZlbnRNb2R1bGUuT3d0RXZlbnQoJ2Rpc2Nvbm5lY3QnKSk7XG4gICAgICAgIH1cbiAgICAgIH0pO1xuICAgICAgdGhpcy5fc29ja2V0Lm9uKCdjb25uZWN0X2Vycm9yJywgKGUpID0+IHtcbiAgICAgICAgcmVqZWN0KGBjb25uZWN0X2Vycm9yOiR7aG9zdH1gKTtcbiAgICAgIH0pO1xuICAgICAgdGhpcy5fc29ja2V0Lm9uKCdkcm9wJywgKCkgPT4ge1xuICAgICAgICB0aGlzLl9yZWNvbm5lY3RUaW1lcyA9IHJlY29ubmVjdGlvbkF0dGVtcHRzO1xuICAgICAgfSk7XG4gICAgICB0aGlzLl9zb2NrZXQub24oJ2Rpc2Nvbm5lY3QnLCAoKSA9PiB7XG4gICAgICAgIHRoaXMuX2NsZWFyUmVjb25uZWN0aW9uVGFzaygpO1xuICAgICAgICBpZiAodGhpcy5fcmVjb25uZWN0VGltZXMgPj0gcmVjb25uZWN0aW9uQXR0ZW1wdHMpIHtcbiAgICAgICAgICB0aGlzLl9sb2dnZWRJbiA9IGZhbHNlO1xuICAgICAgICAgIHRoaXMuZGlzcGF0Y2hFdmVudChuZXcgRXZlbnRNb2R1bGUuT3d0RXZlbnQoJ2Rpc2Nvbm5lY3QnKSk7XG4gICAgICAgIH1cbiAgICAgIH0pO1xuICAgICAgdGhpcy5fc29ja2V0LmVtaXQoJ2xvZ2luJywgbG9naW5JbmZvLCAoc3RhdHVzLCBkYXRhKSA9PiB7XG4gICAgICAgIGlmIChzdGF0dXMgPT09ICdvaycpIHtcbiAgICAgICAgICB0aGlzLl9sb2dnZWRJbiA9IHRydWU7XG4gICAgICAgICAgdGhpcy5fb25SZWNvbm5lY3Rpb25UaWNrZXQoZGF0YS5yZWNvbm5lY3Rpb25UaWNrZXQpO1xuICAgICAgICAgIHRoaXMuX3NvY2tldC5vbignY29ubmVjdCcsICgpID0+IHtcbiAgICAgICAgICAgIC8vIHJlLWxvZ2luIHdpdGggcmVjb25uZWN0aW9uIHRpY2tldC5cbiAgICAgICAgICAgIHRoaXMuX3NvY2tldC5lbWl0KCdyZWxvZ2luJywgdGhpcy5fcmVjb25uZWN0aW9uVGlja2V0LCAoc3RhdHVzLFxuICAgICAgICAgICAgICAgIGRhdGEpID0+IHtcbiAgICAgICAgICAgICAgaWYgKHN0YXR1cyA9PT0gJ29rJykge1xuICAgICAgICAgICAgICAgIHRoaXMuX3JlY29ubmVjdFRpbWVzID0gMDtcbiAgICAgICAgICAgICAgICB0aGlzLl9vblJlY29ubmVjdGlvblRpY2tldChkYXRhKTtcbiAgICAgICAgICAgICAgfSBlbHNlIHtcbiAgICAgICAgICAgICAgICB0aGlzLmRpc3BhdGNoRXZlbnQobmV3IEV2ZW50TW9kdWxlLk93dEV2ZW50KCdkaXNjb25uZWN0JykpO1xuICAgICAgICAgICAgICB9XG4gICAgICAgICAgICB9KTtcbiAgICAgICAgICB9KTtcbiAgICAgICAgfVxuICAgICAgICBoYW5kbGVSZXNwb25zZShzdGF0dXMsIGRhdGEsIHJlc29sdmUsIHJlamVjdCk7XG4gICAgICB9KTtcbiAgICB9KTtcbiAgfVxuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gZGlzY29ubmVjdFxuICAgKiBAaW5zdGFuY2VcbiAgICogQGRlc2MgRGlzY29ubmVjdCBmcm9tIGEgcG9ydGFsLlxuICAgKiBAbWVtYmVyb2YgT21zLkNvbmZlcmVuY2UuU2lvU2lnbmFsaW5nXG4gICAqIEByZXR1cm4ge1Byb21pc2U8T2JqZWN0LCBFcnJvcj59IFJldHVybiBhIHByb21pc2UgcmVzb2x2ZWQgd2l0aCB0aGUgZGF0YSByZXR1cm5lZCBieSBwb3J0YWwgaWYgc3VjY2Vzc2Z1bGx5IGRpc2Nvbm5lY3RlZC4gT3IgcmV0dXJuIGEgcHJvbWlzZSByZWplY3RlZCB3aXRoIGEgbmV3bHkgY3JlYXRlZCBPbXMuRXJyb3IgaWYgZmFpbGVkLlxuICAgKiBAcHJpdmF0ZS5cbiAgICovXG4gIGRpc2Nvbm5lY3QoKSB7XG4gICAgaWYgKCF0aGlzLl9zb2NrZXQgfHwgdGhpcy5fc29ja2V0LmRpc2Nvbm5lY3RlZCkge1xuICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KG5ldyBDb25mZXJlbmNlRXJyb3IoXG4gICAgICAgICAgJ1BvcnRhbCBpcyBub3QgY29ubmVjdGVkLicpKTtcbiAgICB9XG4gICAgcmV0dXJuIG5ldyBQcm9taXNlKChyZXNvbHZlLCByZWplY3QpID0+IHtcbiAgICAgIHRoaXMuX3NvY2tldC5lbWl0KCdsb2dvdXQnLCAoc3RhdHVzLCBkYXRhKSA9PiB7XG4gICAgICAgIC8vIE1heGltaXplIHRoZSByZWNvbm5lY3QgdGltZXMgdG8gZGlzYWJsZSByZWNvbm5lY3Rpb24uXG4gICAgICAgIHRoaXMuX3JlY29ubmVjdFRpbWVzID0gcmVjb25uZWN0aW9uQXR0ZW1wdHM7XG4gICAgICAgIHRoaXMuX3NvY2tldC5kaXNjb25uZWN0KCk7XG4gICAgICAgIGhhbmRsZVJlc3BvbnNlKHN0YXR1cywgZGF0YSwgcmVzb2x2ZSwgcmVqZWN0KTtcbiAgICAgIH0pO1xuICAgIH0pO1xuICB9XG5cbiAgLyoqXG4gICAqIEBmdW5jdGlvbiBzZW5kXG4gICAqIEBpbnN0YW5jZVxuICAgKiBAZGVzYyBTZW5kIGRhdGEgdG8gcG9ydGFsLlxuICAgKiBAbWVtYmVyb2YgT21zLkNvbmZlcmVuY2UuU2lvU2lnbmFsaW5nXG4gICAqIEByZXR1cm4ge1Byb21pc2U8T2JqZWN0LCBFcnJvcj59IFJldHVybiBhIHByb21pc2UgcmVzb2x2ZWQgd2l0aCB0aGUgZGF0YSByZXR1cm5lZCBieSBwb3J0YWwuIE9yIHJldHVybiBhIHByb21pc2UgcmVqZWN0ZWQgd2l0aCBhIG5ld2x5IGNyZWF0ZWQgT21zLkVycm9yIGlmIGZhaWxlZCB0byBzZW5kIHRoZSBtZXNzYWdlLlxuICAgKiBAcGFyYW0ge3N0cmluZ30gcmVxdWVzdE5hbWUgTmFtZSBkZWZpbmVkIGluIGNsaWVudC1zZXJ2ZXIgcHJvdG9jb2wuXG4gICAqIEBwYXJhbSB7c3RyaW5nfSByZXF1ZXN0RGF0YSBEYXRhIGZvcm1hdCBpcyBkZWZpbmVkIGluIGNsaWVudC1zZXJ2ZXIgcHJvdG9jb2wuXG4gICAqIEBwcml2YXRlLlxuICAgKi9cbiAgc2VuZChyZXF1ZXN0TmFtZSwgcmVxdWVzdERhdGEpIHtcbiAgICByZXR1cm4gbmV3IFByb21pc2UoKHJlc29sdmUsIHJlamVjdCkgPT4ge1xuICAgICAgdGhpcy5fc29ja2V0LmVtaXQocmVxdWVzdE5hbWUsIHJlcXVlc3REYXRhLCAoc3RhdHVzLCBkYXRhKSA9PiB7XG4gICAgICAgIGhhbmRsZVJlc3BvbnNlKHN0YXR1cywgZGF0YSwgcmVzb2x2ZSwgcmVqZWN0KTtcbiAgICAgIH0pO1xuICAgIH0pO1xuICB9XG5cbiAgLyoqXG4gICAqIEBmdW5jdGlvbiBfb25SZWNvbm5lY3Rpb25UaWNrZXRcbiAgICogQGluc3RhbmNlXG4gICAqIEBkZXNjIFBhcnNlIHJlY29ubmVjdGlvbiB0aWNrZXQgYW5kIHNjaGVkdWxlIHRpY2tldCByZWZyZXNoaW5nLlxuICAgKiBAcGFyYW0ge3N0cmluZ30gdGlja2V0U3RyaW5nIFJlY29ubmVjdGlvbiB0aWNrZXQuXG4gICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5TaW9TaWduYWxpbmdcbiAgICogQHByaXZhdGUuXG4gICAqL1xuICBfb25SZWNvbm5lY3Rpb25UaWNrZXQodGlja2V0U3RyaW5nKSB7XG4gICAgdGhpcy5fcmVjb25uZWN0aW9uVGlja2V0ID0gdGlja2V0U3RyaW5nO1xuICAgIGNvbnN0IHRpY2tldCA9IEpTT04ucGFyc2UoQmFzZTY0LmRlY29kZUJhc2U2NCh0aWNrZXRTdHJpbmcpKTtcbiAgICAvLyBSZWZyZXNoIHRpY2tldCAxIG1pbiBvciAxMCBzZWNvbmRzIGJlZm9yZSBpdCBleHBpcmVzLlxuICAgIGNvbnN0IG5vdyA9IERhdGUubm93KCk7XG4gICAgY29uc3QgbWlsbGlzZWNvbmRzSW5PbmVNaW51dGUgPSA2MCAqIDEwMDA7XG4gICAgY29uc3QgbWlsbGlzZWNvbmRzSW5UZW5TZWNvbmRzID0gMTAgKiAxMDAwO1xuICAgIGlmICh0aWNrZXQubm90QWZ0ZXIgPD0gbm93IC0gbWlsbGlzZWNvbmRzSW5UZW5TZWNvbmRzKSB7XG4gICAgICBMb2dnZXIud2FybmluZygnUmVjb25uZWN0aW9uIHRpY2tldCBleHBpcmVzIHRvbyBzb29uLicpO1xuICAgICAgcmV0dXJuO1xuICAgIH1cbiAgICBsZXQgcmVmcmVzaEFmdGVyID0gdGlja2V0Lm5vdEFmdGVyIC0gbm93IC0gbWlsbGlzZWNvbmRzSW5PbmVNaW51dGU7XG4gICAgaWYgKHJlZnJlc2hBZnRlciA8IDApIHtcbiAgICAgIHJlZnJlc2hBZnRlciA9IHRpY2tldC5ub3RBZnRlciAtIG5vdyAtIG1pbGxpc2Vjb25kc0luVGVuU2Vjb25kcztcbiAgICB9XG4gICAgdGhpcy5fY2xlYXJSZWNvbm5lY3Rpb25UYXNrKCk7XG4gICAgdGhpcy5fcmVmcmVzaFJlY29ubmVjdGlvblRpY2tldCA9IHNldFRpbWVvdXQoKCkgPT4ge1xuICAgICAgdGhpcy5fc29ja2V0LmVtaXQoJ3JlZnJlc2hSZWNvbm5lY3Rpb25UaWNrZXQnLCAoc3RhdHVzLCBkYXRhKSA9PiB7XG4gICAgICAgIGlmIChzdGF0dXMgIT09ICdvaycpIHtcbiAgICAgICAgICBMb2dnZXIud2FybmluZygnRmFpbGVkIHRvIHJlZnJlc2ggcmVjb25uZWN0aW9uIHRpY2tldC4nKTtcbiAgICAgICAgICByZXR1cm47XG4gICAgICAgIH1cbiAgICAgICAgdGhpcy5fb25SZWNvbm5lY3Rpb25UaWNrZXQoZGF0YSk7XG4gICAgICB9KTtcbiAgICB9LCByZWZyZXNoQWZ0ZXIpO1xuICB9XG5cbiAgLyoqXG4gICAqIEBmdW5jdGlvbiBfY2xlYXJSZWNvbm5lY3Rpb25UYXNrXG4gICAqIEBpbnN0YW5jZVxuICAgKiBAZGVzYyBTdG9wIHRyeWluZyB0byByZWZyZXNoIHJlY29ubmVjdGlvbiB0aWNrZXQuXG4gICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5TaW9TaWduYWxpbmdcbiAgICogQHByaXZhdGUuXG4gICAqL1xuICBfY2xlYXJSZWNvbm5lY3Rpb25UYXNrKCkge1xuICAgIGNsZWFyVGltZW91dCh0aGlzLl9yZWZyZXNoUmVjb25uZWN0aW9uVGlja2V0KTtcbiAgICB0aGlzLl9yZWZyZXNoUmVjb25uZWN0aW9uVGlja2V0ID0gbnVsbDtcbiAgfVxufVxuIiwiLy8gQ29weXJpZ2h0IChDKSA8MjAxOD4gSW50ZWwgQ29ycG9yYXRpb25cbi8vXG4vLyBTUERYLUxpY2Vuc2UtSWRlbnRpZmllcjogQXBhY2hlLTIuMFxuXG4vLyBUaGlzIGZpbGUgZG9lc24ndCBoYXZlIHB1YmxpYyBBUElzLlxuLyogZXNsaW50LWRpc2FibGUgdmFsaWQtanNkb2MgKi9cblxuJ3VzZSBzdHJpY3QnO1xuXG5pbXBvcnQgKiBhcyBQdWJsaWNhdGlvbk1vZHVsZSBmcm9tICcuLi9iYXNlL3B1YmxpY2F0aW9uLmpzJztcbmltcG9ydCAqIGFzIE1lZGlhRm9ybWF0TW9kdWxlIGZyb20gJy4uL2Jhc2UvbWVkaWFmb3JtYXQuanMnO1xuaW1wb3J0ICogYXMgQ29kZWNNb2R1bGUgZnJvbSAnLi4vYmFzZS9jb2RlYy5qcyc7XG5pbXBvcnQgKiBhcyBTdWJzY3JpcHRpb25Nb2R1bGUgZnJvbSAnLi9zdWJzY3JpcHRpb24uanMnO1xuaW1wb3J0IExvZ2dlciBmcm9tICcuLi9iYXNlL2xvZ2dlci5qcyc7XG5cbi8qKlxuICogQGZ1bmN0aW9uIGV4dHJhY3RCaXRyYXRlTXVsdGlwbGllclxuICogQGRlc2MgRXh0cmFjdCBiaXRyYXRlIG11bHRpcGxpZXIgZnJvbSBhIHN0cmluZyBsaWtlIFwieDAuMlwiLlxuICogQHJldHVybiB7UHJvbWlzZTxPYmplY3QsIEVycm9yPn0gVGhlIGZsb2F0IG51bWJlciBhZnRlciBcInhcIi5cbiAqIEBwcml2YXRlXG4gKi9cbmZ1bmN0aW9uIGV4dHJhY3RCaXRyYXRlTXVsdGlwbGllcihpbnB1dCkge1xuICBpZiAodHlwZW9mIGlucHV0ICE9PSAnc3RyaW5nJyB8fCAhaW5wdXQuc3RhcnRzV2l0aCgneCcpKSB7XG4gICAgTG9nZ2VyLndhcm5pbmcoJ0ludmFsaWQgYml0cmF0ZSBtdWx0aXBsaWVyIGlucHV0LicpO1xuICAgIHJldHVybiAwO1xuICB9XG4gIHJldHVybiBOdW1iZXIucGFyc2VGbG9hdChpbnB1dC5yZXBsYWNlKC9eeC8sICcnKSk7XG59XG5cbi8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG5mdW5jdGlvbiBzb3J0TnVtYmVycyh4LCB5KSB7XG4gIHJldHVybiB4IC0geTtcbn1cblxuLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbmZ1bmN0aW9uIHNvcnRSZXNvbHV0aW9ucyh4LCB5KSB7XG4gIGlmICh4LndpZHRoICE9PSB5LndpZHRoKSB7XG4gICAgcmV0dXJuIHgud2lkdGggLSB5LndpZHRoO1xuICB9IGVsc2Uge1xuICAgIHJldHVybiB4LmhlaWdodCAtIHkuaGVpZ2h0O1xuICB9XG59XG5cbi8qKlxuICogQGZ1bmN0aW9uIGNvbnZlcnRUb1B1YmxpY2F0aW9uU2V0dGluZ3NcbiAqIEBkZXNjIENvbnZlcnQgbWVkaWFJbmZvIHJlY2VpdmVkIGZyb20gc2VydmVyIHRvIFB1YmxpY2F0aW9uU2V0dGluZ3MuXG4gKiBAcHJpdmF0ZVxuICovXG5leHBvcnQgZnVuY3Rpb24gY29udmVydFRvUHVibGljYXRpb25TZXR0aW5ncyhtZWRpYUluZm8pIHtcbiAgY29uc3QgYXVkaW8gPSBbXTtcbiAgY29uc3QgdmlkZW8gPSBbXTtcbiAgbGV0IGF1ZGlvQ29kZWM7XG4gIGxldCB2aWRlb0NvZGVjO1xuICBsZXQgcmVzb2x1dGlvbjtcbiAgbGV0IGZyYW1lcmF0ZTtcbiAgbGV0IGJpdHJhdGU7XG4gIGxldCBrZXlGcmFtZUludGVydmFsO1xuICBsZXQgcmlkO1xuICBmb3IgKGNvbnN0IHRyYWNrIG9mIG1lZGlhSW5mby50cmFja3MpIHtcbiAgICBpZiAodHJhY2sudHlwZSA9PT0gJ2F1ZGlvJykge1xuICAgICAgaWYgKHRyYWNrLmZvcm1hdCkge1xuICAgICAgICBhdWRpb0NvZGVjID0gbmV3IENvZGVjTW9kdWxlLkF1ZGlvQ29kZWNQYXJhbWV0ZXJzKFxuICAgICAgICAgICAgdHJhY2suZm9ybWF0LmNvZGVjLCB0cmFjay5mb3JtYXQuY2hhbm5lbE51bSxcbiAgICAgICAgICAgIHRyYWNrLmZvcm1hdC5zYW1wbGVSYXRlKTtcbiAgICAgIH1cbiAgICAgIGNvbnN0IGF1ZGlvUHVibGljYXRpb25TZXR0aW5ncyA9XG4gICAgICAgICAgbmV3IFB1YmxpY2F0aW9uTW9kdWxlLkF1ZGlvUHVibGljYXRpb25TZXR0aW5ncyhhdWRpb0NvZGVjKTtcbiAgICAgIGF1ZGlvUHVibGljYXRpb25TZXR0aW5ncy5fdHJhY2tJZCA9IHRyYWNrLmlkO1xuICAgICAgYXVkaW8ucHVzaChhdWRpb1B1YmxpY2F0aW9uU2V0dGluZ3MpO1xuICAgIH0gZWxzZSBpZiAodHJhY2sudHlwZSA9PT0gJ3ZpZGVvJykge1xuICAgICAgaWYgKHRyYWNrLmZvcm1hdCkge1xuICAgICAgICB2aWRlb0NvZGVjID0gbmV3IENvZGVjTW9kdWxlLlZpZGVvQ29kZWNQYXJhbWV0ZXJzKFxuICAgICAgICAgICAgdHJhY2suZm9ybWF0LmNvZGVjLCB0cmFjay5mb3JtYXQucHJvZmlsZSk7XG4gICAgICB9XG4gICAgICBpZiAodHJhY2sucGFyYW1ldGVycykge1xuICAgICAgICBpZiAodHJhY2sucGFyYW1ldGVycy5yZXNvbHV0aW9uKSB7XG4gICAgICAgICAgcmVzb2x1dGlvbiA9IG5ldyBNZWRpYUZvcm1hdE1vZHVsZS5SZXNvbHV0aW9uKFxuICAgICAgICAgICAgICB0cmFjay5wYXJhbWV0ZXJzLnJlc29sdXRpb24ud2lkdGgsXG4gICAgICAgICAgICAgIHRyYWNrLnBhcmFtZXRlcnMucmVzb2x1dGlvbi5oZWlnaHQpO1xuICAgICAgICB9XG4gICAgICAgIGZyYW1lcmF0ZSA9IHRyYWNrLnBhcmFtZXRlcnMuZnJhbWVyYXRlO1xuICAgICAgICBiaXRyYXRlID0gdHJhY2sucGFyYW1ldGVycy5iaXRyYXRlICogMTAwMDtcbiAgICAgICAga2V5RnJhbWVJbnRlcnZhbCA9IHRyYWNrLnBhcmFtZXRlcnMua2V5RnJhbWVJbnRlcnZhbDtcbiAgICAgIH1cbiAgICAgIGlmICh0cmFjay5yaWQpIHtcbiAgICAgICAgcmlkID0gdHJhY2sucmlkO1xuICAgICAgfVxuICAgICAgY29uc3QgdmlkZW9QdWJsaWNhdGlvblNldHRpbmdzID1cbiAgICAgICAgICBuZXcgUHVibGljYXRpb25Nb2R1bGUuVmlkZW9QdWJsaWNhdGlvblNldHRpbmdzKFxuICAgICAgICAgICAgICB2aWRlb0NvZGVjLCByZXNvbHV0aW9uLCBmcmFtZXJhdGUsIGJpdHJhdGUsXG4gICAgICAgICAgICAgIGtleUZyYW1lSW50ZXJ2YWwsIHJpZCk7XG4gICAgICB2aWRlb1B1YmxpY2F0aW9uU2V0dGluZ3MuX3RyYWNrSWQgPSB0cmFjay5pZDtcbiAgICAgIHZpZGVvLnB1c2godmlkZW9QdWJsaWNhdGlvblNldHRpbmdzKTtcbiAgICB9XG4gIH1cblxuICByZXR1cm4gbmV3IFB1YmxpY2F0aW9uTW9kdWxlLlB1YmxpY2F0aW9uU2V0dGluZ3MoYXVkaW8sIHZpZGVvKTtcbn1cblxuLyoqXG4gKiBAZnVuY3Rpb24gY29udmVydFRvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzXG4gKiBAZGVzYyBDb252ZXJ0IG1lZGlhSW5mbyByZWNlaXZlZCBmcm9tIHNlcnZlciB0byBTdWJzY3JpcHRpb25DYXBhYmlsaXRpZXMuXG4gKiBAcHJpdmF0ZVxuICovXG5leHBvcnQgZnVuY3Rpb24gY29udmVydFRvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzKG1lZGlhSW5mbykge1xuICBsZXQgYXVkaW87XG4gIGxldCB2aWRlbztcblxuICBmb3IgKGNvbnN0IHRyYWNrIG9mIG1lZGlhSW5mby50cmFja3MpIHtcbiAgICBpZiAodHJhY2sudHlwZSA9PT0gJ2F1ZGlvJykge1xuICAgICAgY29uc3QgYXVkaW9Db2RlY3MgPSBbXTtcbiAgICAgIGlmICh0cmFjay5vcHRpb25hbCAmJiB0cmFjay5vcHRpb25hbC5mb3JtYXQpIHtcbiAgICAgICAgZm9yIChjb25zdCBhdWRpb0NvZGVjSW5mbyBvZiB0cmFjay5vcHRpb25hbC5mb3JtYXQpIHtcbiAgICAgICAgICBjb25zdCBhdWRpb0NvZGVjID0gbmV3IENvZGVjTW9kdWxlLkF1ZGlvQ29kZWNQYXJhbWV0ZXJzKFxuICAgICAgICAgICAgICBhdWRpb0NvZGVjSW5mby5jb2RlYywgYXVkaW9Db2RlY0luZm8uY2hhbm5lbE51bSxcbiAgICAgICAgICAgICAgYXVkaW9Db2RlY0luZm8uc2FtcGxlUmF0ZSk7XG4gICAgICAgICAgYXVkaW9Db2RlY3MucHVzaChhdWRpb0NvZGVjKTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgICAgYXVkaW9Db2RlY3Muc29ydCgpO1xuICAgICAgYXVkaW8gPSBuZXcgU3Vic2NyaXB0aW9uTW9kdWxlLkF1ZGlvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzKGF1ZGlvQ29kZWNzKTtcbiAgICB9IGVsc2UgaWYgKHRyYWNrLnR5cGUgPT09ICd2aWRlbycpIHtcbiAgICAgIGNvbnN0IHZpZGVvQ29kZWNzID0gW107XG4gICAgICBpZiAodHJhY2sub3B0aW9uYWwgJiYgdHJhY2sub3B0aW9uYWwuZm9ybWF0KSB7XG4gICAgICAgIGZvciAoY29uc3QgdmlkZW9Db2RlY0luZm8gb2YgdHJhY2sub3B0aW9uYWwuZm9ybWF0KSB7XG4gICAgICAgICAgY29uc3QgdmlkZW9Db2RlYyA9IG5ldyBDb2RlY01vZHVsZS5WaWRlb0NvZGVjUGFyYW1ldGVycyhcbiAgICAgICAgICAgICAgdmlkZW9Db2RlY0luZm8uY29kZWMsIHZpZGVvQ29kZWNJbmZvLnByb2ZpbGUpO1xuICAgICAgICAgIHZpZGVvQ29kZWNzLnB1c2godmlkZW9Db2RlYyk7XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICAgIHZpZGVvQ29kZWNzLnNvcnQoKTtcbiAgICAgIGNvbnN0IHJlc29sdXRpb25zID0gQXJyYXkuZnJvbShcbiAgICAgICAgICB0cmFjay5vcHRpb25hbC5wYXJhbWV0ZXJzLnJlc29sdXRpb24sXG4gICAgICAgICAgKHIpID0+IG5ldyBNZWRpYUZvcm1hdE1vZHVsZS5SZXNvbHV0aW9uKHIud2lkdGgsIHIuaGVpZ2h0KSk7XG4gICAgICByZXNvbHV0aW9ucy5zb3J0KHNvcnRSZXNvbHV0aW9ucyk7XG4gICAgICBjb25zdCBiaXRyYXRlcyA9IEFycmF5LmZyb20oXG4gICAgICAgICAgdHJhY2sub3B0aW9uYWwucGFyYW1ldGVycy5iaXRyYXRlLFxuICAgICAgICAgIChiaXRyYXRlKSA9PiBleHRyYWN0Qml0cmF0ZU11bHRpcGxpZXIoYml0cmF0ZSkpO1xuICAgICAgYml0cmF0ZXMucHVzaCgxLjApO1xuICAgICAgYml0cmF0ZXMuc29ydChzb3J0TnVtYmVycyk7XG4gICAgICBjb25zdCBmcmFtZVJhdGVzID0gSlNPTi5wYXJzZShcbiAgICAgICAgICBKU09OLnN0cmluZ2lmeSh0cmFjay5vcHRpb25hbC5wYXJhbWV0ZXJzLmZyYW1lcmF0ZSkpO1xuICAgICAgZnJhbWVSYXRlcy5zb3J0KHNvcnROdW1iZXJzKTtcbiAgICAgIGNvbnN0IGtleUZyYW1lSW50ZXJ2YWxzID0gSlNPTi5wYXJzZShcbiAgICAgICAgICBKU09OLnN0cmluZ2lmeSh0cmFjay5vcHRpb25hbC5wYXJhbWV0ZXJzLmtleUZyYW1lSW50ZXJ2YWwpKTtcbiAgICAgIGtleUZyYW1lSW50ZXJ2YWxzLnNvcnQoc29ydE51bWJlcnMpO1xuICAgICAgdmlkZW8gPSBuZXcgU3Vic2NyaXB0aW9uTW9kdWxlLlZpZGVvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzKFxuICAgICAgICAgIHZpZGVvQ29kZWNzLCByZXNvbHV0aW9ucywgZnJhbWVSYXRlcywgYml0cmF0ZXMsIGtleUZyYW1lSW50ZXJ2YWxzKTtcbiAgICB9XG4gIH1cbiAgcmV0dXJuIG5ldyBTdWJzY3JpcHRpb25Nb2R1bGUuU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzKGF1ZGlvLCB2aWRlbyk7XG59XG4iLCIvLyBDb3B5cmlnaHQgKEMpIDwyMDE4PiBJbnRlbCBDb3Jwb3JhdGlvblxuLy9cbi8vIFNQRFgtTGljZW5zZS1JZGVudGlmaWVyOiBBcGFjaGUtMi4wXG5cbid1c2Ugc3RyaWN0JztcblxuaW1wb3J0IHtFdmVudERpc3BhdGNoZXJ9IGZyb20gJy4uL2Jhc2UvZXZlbnQuanMnO1xuXG4vKipcbiAqIEBjbGFzcyBBdWRpb1N1YnNjcmlwdGlvbkNhcGFiaWxpdGllc1xuICogQG1lbWJlck9mIE93dC5Db25mZXJlbmNlXG4gKiBAY2xhc3NEZXNjIFJlcHJlc2VudHMgdGhlIGF1ZGlvIGNhcGFiaWxpdHkgZm9yIHN1YnNjcmlwdGlvbi5cbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNsYXNzIEF1ZGlvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzIHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IoY29kZWNzKSB7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7QXJyYXkuPE93dC5CYXNlLkF1ZGlvQ29kZWNQYXJhbWV0ZXJzPn0gY29kZWNzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLkF1ZGlvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzXG4gICAgICovXG4gICAgdGhpcy5jb2RlY3MgPSBjb2RlY3M7XG4gIH1cbn1cblxuLyoqXG4gKiBAY2xhc3MgVmlkZW9TdWJzY3JpcHRpb25DYXBhYmlsaXRpZXNcbiAqIEBtZW1iZXJPZiBPd3QuQ29uZmVyZW5jZVxuICogQGNsYXNzRGVzYyBSZXByZXNlbnRzIHRoZSB2aWRlbyBjYXBhYmlsaXR5IGZvciBzdWJzY3JpcHRpb24uXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKi9cbmV4cG9ydCBjbGFzcyBWaWRlb1N1YnNjcmlwdGlvbkNhcGFiaWxpdGllcyB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKGNvZGVjcywgcmVzb2x1dGlvbnMsIGZyYW1lUmF0ZXMsIGJpdHJhdGVNdWx0aXBsaWVycyxcbiAgICAgIGtleUZyYW1lSW50ZXJ2YWxzKSB7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7QXJyYXkuPE93dC5CYXNlLlZpZGVvQ29kZWNQYXJhbWV0ZXJzPn0gY29kZWNzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlZpZGVvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzXG4gICAgICovXG4gICAgdGhpcy5jb2RlY3MgPSBjb2RlY3M7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7QXJyYXkuPE93dC5CYXNlLlJlc29sdXRpb24+fSByZXNvbHV0aW9uc1xuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5WaWRlb1N1YnNjcmlwdGlvbkNhcGFiaWxpdGllc1xuICAgICAqL1xuICAgIHRoaXMucmVzb2x1dGlvbnMgPSByZXNvbHV0aW9ucztcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHtBcnJheS48bnVtYmVyPn0gZnJhbWVSYXRlc1xuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5WaWRlb1N1YnNjcmlwdGlvbkNhcGFiaWxpdGllc1xuICAgICAqL1xuICAgIHRoaXMuZnJhbWVSYXRlcyA9IGZyYW1lUmF0ZXM7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7QXJyYXkuPG51bWJlcj59IGJpdHJhdGVNdWx0aXBsaWVyc1xuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5WaWRlb1N1YnNjcmlwdGlvbkNhcGFiaWxpdGllc1xuICAgICAqL1xuICAgIHRoaXMuYml0cmF0ZU11bHRpcGxpZXJzID0gYml0cmF0ZU11bHRpcGxpZXJzO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIge0FycmF5LjxudW1iZXI+fSBrZXlGcmFtZUludGVydmFsc1xuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5WaWRlb1N1YnNjcmlwdGlvbkNhcGFiaWxpdGllc1xuICAgICAqL1xuICAgIHRoaXMua2V5RnJhbWVJbnRlcnZhbHMgPSBrZXlGcmFtZUludGVydmFscztcbiAgfVxufVxuXG4vKipcbiAqIEBjbGFzcyBTdWJzY3JpcHRpb25DYXBhYmlsaXRpZXNcbiAqIEBtZW1iZXJPZiBPd3QuQ29uZmVyZW5jZVxuICogQGNsYXNzRGVzYyBSZXByZXNlbnRzIHRoZSBjYXBhYmlsaXR5IGZvciBzdWJzY3JpcHRpb24uXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKi9cbmV4cG9ydCBjbGFzcyBTdWJzY3JpcHRpb25DYXBhYmlsaXRpZXMge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3RvcihhdWRpbywgdmlkZW8pIHtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/T3d0LkNvbmZlcmVuY2UuQXVkaW9TdWJzY3JpcHRpb25DYXBhYmlsaXRpZXN9IGF1ZGlvXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlN1YnNjcmlwdGlvbkNhcGFiaWxpdGllc1xuICAgICAqL1xuICAgIHRoaXMuYXVkaW8gPSBhdWRpbztcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/T3d0LkNvbmZlcmVuY2UuVmlkZW9TdWJzY3JpcHRpb25DYXBhYmlsaXRpZXN9IHZpZGVvXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlN1YnNjcmlwdGlvbkNhcGFiaWxpdGllc1xuICAgICAqL1xuICAgIHRoaXMudmlkZW8gPSB2aWRlbztcbiAgfVxufVxuXG4vKipcbiAqIEBjbGFzcyBBdWRpb1N1YnNjcmlwdGlvbkNvbnN0cmFpbnRzXG4gKiBAbWVtYmVyT2YgT3d0LkNvbmZlcmVuY2VcbiAqIEBjbGFzc0Rlc2MgUmVwcmVzZW50cyB0aGUgYXVkaW8gY29uc3RyYWludHMgZm9yIHN1YnNjcmlwdGlvbi5cbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNsYXNzIEF1ZGlvU3Vic2NyaXB0aW9uQ29uc3RyYWludHMge1xuICAvLyBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvY1xuICBjb25zdHJ1Y3Rvcihjb2RlY3MpIHtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/QXJyYXkuPE93dC5CYXNlLkF1ZGlvQ29kZWNQYXJhbWV0ZXJzPn0gY29kZWNzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLkF1ZGlvU3Vic2NyaXB0aW9uQ29uc3RyYWludHNcbiAgICAgKiBAZGVzYyBDb2RlY3MgYWNjZXB0ZWQuIElmIG5vbmUgb2YgYGNvZGVjc2Agc3VwcG9ydGVkIGJ5IGJvdGggc2lkZXMsIGNvbm5lY3Rpb24gZmFpbHMuIExlYXZlIGl0IHVuZGVmaW5lZCB3aWxsIHVzZSBhbGwgcG9zc2libGUgY29kZWNzLlxuICAgICAqL1xuICAgIHRoaXMuY29kZWNzID0gY29kZWNzO1xuICB9XG59XG5cbi8qKlxuICogQGNsYXNzIFZpZGVvU3Vic2NyaXB0aW9uQ29uc3RyYWludHNcbiAqIEBtZW1iZXJPZiBPd3QuQ29uZmVyZW5jZVxuICogQGNsYXNzRGVzYyBSZXByZXNlbnRzIHRoZSB2aWRlbyBjb25zdHJhaW50cyBmb3Igc3Vic2NyaXB0aW9uLlxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY2xhc3MgVmlkZW9TdWJzY3JpcHRpb25Db25zdHJhaW50cyB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKGNvZGVjcywgcmVzb2x1dGlvbiwgZnJhbWVSYXRlLCBiaXRyYXRlTXVsdGlwbGllcixcbiAgICAgIGtleUZyYW1lSW50ZXJ2YWwsIHJpZCkge1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIgez9BcnJheS48T3d0LkJhc2UuVmlkZW9Db2RlY1BhcmFtZXRlcnM+fSBjb2RlY3NcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2UuVmlkZW9TdWJzY3JpcHRpb25Db25zdHJhaW50c1xuICAgICAqIEBkZXNjIENvZGVjcyBhY2NlcHRlZC4gSWYgbm9uZSBvZiBgY29kZWNzYCBzdXBwb3J0ZWQgYnkgYm90aCBzaWRlcywgY29ubmVjdGlvbiBmYWlscy4gTGVhdmUgaXQgdW5kZWZpbmVkIHdpbGwgdXNlIGFsbCBwb3NzaWJsZSBjb2RlY3MuXG4gICAgICovXG4gICAgdGhpcy5jb2RlY3MgPSBjb2RlY3M7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P093dC5CYXNlLlJlc29sdXRpb259IHJlc29sdXRpb25cbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2UuVmlkZW9TdWJzY3JpcHRpb25Db25zdHJhaW50c1xuICAgICAqIEBkZXNjIE9ubHkgcmVzb2x1dGlvbnMgbGlzdGVkIGluIE93dC5Db25mZXJlbmNlLlZpZGVvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzIGFyZSBhbGxvd2VkLlxuICAgICAqL1xuICAgIHRoaXMucmVzb2x1dGlvbiA9IHJlc29sdXRpb247XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P251bWJlcn0gZnJhbWVSYXRlXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlZpZGVvU3Vic2NyaXB0aW9uQ29uc3RyYWludHNcbiAgICAgKiBAZGVzYyBPbmx5IGZyYW1lUmF0ZXMgbGlzdGVkIGluIE93dC5Db25mZXJlbmNlLlZpZGVvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzIGFyZSBhbGxvd2VkLlxuICAgICAqL1xuICAgIHRoaXMuZnJhbWVSYXRlID0gZnJhbWVSYXRlO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIgez9udW1iZXJ9IGJpdHJhdGVNdWx0aXBsaWVyXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlZpZGVvU3Vic2NyaXB0aW9uQ29uc3RyYWludHNcbiAgICAgKiBAZGVzYyBPbmx5IGJpdHJhdGVNdWx0aXBsaWVycyBsaXN0ZWQgaW4gT3d0LkNvbmZlcmVuY2UuVmlkZW9TdWJzY3JpcHRpb25DYXBhYmlsaXRpZXMgYXJlIGFsbG93ZWQuXG4gICAgICovXG4gICAgdGhpcy5iaXRyYXRlTXVsdGlwbGllciA9IGJpdHJhdGVNdWx0aXBsaWVyO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIgez9udW1iZXJ9IGtleUZyYW1lSW50ZXJ2YWxcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2UuVmlkZW9TdWJzY3JpcHRpb25Db25zdHJhaW50c1xuICAgICAqIEBkZXNjIE9ubHkga2V5RnJhbWVJbnRlcnZhbHMgbGlzdGVkIGluIE93dC5Db25mZXJlbmNlLlZpZGVvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzIGFyZSBhbGxvd2VkLlxuICAgICAqL1xuICAgIHRoaXMua2V5RnJhbWVJbnRlcnZhbCA9IGtleUZyYW1lSW50ZXJ2YWw7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P251bWJlcn0gcmlkXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlZpZGVvU3Vic2NyaXB0aW9uQ29uc3RyYWludHNcbiAgICAgKiBAZGVzYyBSZXN0cmljdGlvbiBpZGVudGlmaWVyIHRvIGlkZW50aWZ5IHRoZSBSVFAgU3RyZWFtcyB3aXRoaW4gYW4gUlRQIHNlc3Npb24uIFdoZW4gcmlkIGlzIHNwZWNpZmllZCwgb3RoZXIgY29uc3RyYWludHMgd2lsbCBiZSBpZ25vcmVkLlxuICAgICAqL1xuICAgIHRoaXMucmlkID0gcmlkO1xuICB9XG59XG5cbi8qKlxuICogQGNsYXNzIFN1YnNjcmliZU9wdGlvbnNcbiAqIEBtZW1iZXJPZiBPd3QuQ29uZmVyZW5jZVxuICogQGNsYXNzRGVzYyBTdWJzY3JpYmVPcHRpb25zIGRlZmluZXMgb3B0aW9ucyBmb3Igc3Vic2NyaWJpbmcgYSBPd3QuQmFzZS5SZW1vdGVTdHJlYW0uXG4gKi9cbmV4cG9ydCBjbGFzcyBTdWJzY3JpYmVPcHRpb25zIHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IoYXVkaW8sIHZpZGVvLCB0cmFuc3BvcnQpIHtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/T3d0LkNvbmZlcmVuY2UuQXVkaW9TdWJzY3JpcHRpb25Db25zdHJhaW50c30gYXVkaW9cbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2UuU3Vic2NyaWJlT3B0aW9uc1xuICAgICAqL1xuICAgIHRoaXMuYXVkaW8gPSBhdWRpbztcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/T3d0LkNvbmZlcmVuY2UuVmlkZW9TdWJzY3JpcHRpb25Db25zdHJhaW50c30gdmlkZW9cbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2UuU3Vic2NyaWJlT3B0aW9uc1xuICAgICAqL1xuICAgIHRoaXMudmlkZW8gPSB2aWRlbztcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/T3d0LkJhc2UuVHJhbnNwb3J0Q29uc3RyYWludHN9IHRyYW5zcG9ydFxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5TdWJzY3JpYmVPcHRpb25zXG4gICAgICovXG4gICAgdGhpcy50cmFuc3BvcnQgPSB0cmFuc3BvcnQ7XG4gIH1cbn1cblxuLyoqXG4gKiBAY2xhc3MgVmlkZW9TdWJzY3JpcHRpb25VcGRhdGVPcHRpb25zXG4gKiBAbWVtYmVyT2YgT3d0LkNvbmZlcmVuY2VcbiAqIEBjbGFzc0Rlc2MgVmlkZW9TdWJzY3JpcHRpb25VcGRhdGVPcHRpb25zIGRlZmluZXMgb3B0aW9ucyBmb3IgdXBkYXRpbmcgYSBzdWJzY3JpcHRpb24ncyB2aWRlbyBwYXJ0LlxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY2xhc3MgVmlkZW9TdWJzY3JpcHRpb25VcGRhdGVPcHRpb25zIHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IoKSB7XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7P093dC5CYXNlLlJlc29sdXRpb259IHJlc29sdXRpb25cbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2UuVmlkZW9TdWJzY3JpcHRpb25VcGRhdGVPcHRpb25zXG4gICAgICogQGRlc2MgT25seSByZXNvbHV0aW9ucyBsaXN0ZWQgaW4gVmlkZW9TdWJzY3JpcHRpb25DYXBhYmlsaXRpZXMgYXJlIGFsbG93ZWQuXG4gICAgICovXG4gICAgdGhpcy5yZXNvbHV0aW9uID0gdW5kZWZpbmVkO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIgez9udW1iZXJ9IGZyYW1lUmF0ZXNcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2UuVmlkZW9TdWJzY3JpcHRpb25VcGRhdGVPcHRpb25zXG4gICAgICogQGRlc2MgT25seSBmcmFtZVJhdGVzIGxpc3RlZCBpbiBWaWRlb1N1YnNjcmlwdGlvbkNhcGFiaWxpdGllcyBhcmUgYWxsb3dlZC5cbiAgICAgKi9cbiAgICB0aGlzLmZyYW1lUmF0ZSA9IHVuZGVmaW5lZDtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHs/bnVtYmVyfSBiaXRyYXRlTXVsdGlwbGllcnNcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2UuVmlkZW9TdWJzY3JpcHRpb25VcGRhdGVPcHRpb25zXG4gICAgICogQGRlc2MgT25seSBiaXRyYXRlTXVsdGlwbGllcnMgbGlzdGVkIGluIFZpZGVvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzIGFyZSBhbGxvd2VkLlxuICAgICAqL1xuICAgIHRoaXMuYml0cmF0ZU11bHRpcGxpZXJzID0gdW5kZWZpbmVkO1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIgez9udW1iZXJ9IGtleUZyYW1lSW50ZXJ2YWxzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlZpZGVvU3Vic2NyaXB0aW9uVXBkYXRlT3B0aW9uc1xuICAgICAqIEBkZXNjIE9ubHkga2V5RnJhbWVJbnRlcnZhbHMgbGlzdGVkIGluIFZpZGVvU3Vic2NyaXB0aW9uQ2FwYWJpbGl0aWVzIGFyZSBhbGxvd2VkLlxuICAgICAqL1xuICAgIHRoaXMua2V5RnJhbWVJbnRlcnZhbCA9IHVuZGVmaW5lZDtcbiAgfVxufVxuXG4vKipcbiAqIEBjbGFzcyBTdWJzY3JpcHRpb25VcGRhdGVPcHRpb25zXG4gKiBAbWVtYmVyT2YgT3d0LkNvbmZlcmVuY2VcbiAqIEBjbGFzc0Rlc2MgU3Vic2NyaXB0aW9uVXBkYXRlT3B0aW9ucyBkZWZpbmVzIG9wdGlvbnMgZm9yIHVwZGF0aW5nIGEgc3Vic2NyaXB0aW9uLlxuICogQGhpZGVjb25zdHJ1Y3RvclxuICovXG5leHBvcnQgY2xhc3MgU3Vic2NyaXB0aW9uVXBkYXRlT3B0aW9ucyB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKCkge1xuICAgIC8qKlxuICAgICAqIEBtZW1iZXIgez9WaWRlb1N1YnNjcmlwdGlvblVwZGF0ZU9wdGlvbnN9IHZpZGVvXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlN1YnNjcmlwdGlvblVwZGF0ZU9wdGlvbnNcbiAgICAgKi9cbiAgICB0aGlzLnZpZGVvID0gdW5kZWZpbmVkO1xuICB9XG59XG5cbi8qKlxuICogQGNsYXNzIFN1YnNjcmlwdGlvblxuICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlXG4gKiBAY2xhc3NEZXNjIFN1YnNjcmlwdGlvbiBpcyBhIHJlY2VpdmVyIGZvciByZWNlaXZpbmcgYSBzdHJlYW0uXG4gKiBFdmVudHM6XG4gKlxuICogfCBFdmVudCBOYW1lICAgICAgfCBBcmd1bWVudCBUeXBlICAgIHwgRmlyZWQgd2hlbiAgICAgICB8XG4gKiB8IC0tLS0tLS0tLS0tLS0tLS18IC0tLS0tLS0tLS0tLS0tLS0gfCAtLS0tLS0tLS0tLS0tLS0tIHxcbiAqIHwgZW5kZWQgICAgICAgICAgIHwgRXZlbnQgICAgICAgICAgICB8IFN1YnNjcmlwdGlvbiBpcyBlbmRlZC4gfFxuICogfCBlcnJvciAgICAgICAgICAgfCBFcnJvckV2ZW50ICAgICAgIHwgQW4gZXJyb3Igb2NjdXJyZWQgb24gdGhlIHN1YnNjcmlwdGlvbi4gfFxuICogfCBtdXRlICAgICAgICAgICAgfCBNdXRlRXZlbnQgICAgICAgIHwgUHVibGljYXRpb24gaXMgbXV0ZWQuIFJlbW90ZSBzaWRlIHN0b3BwZWQgc2VuZGluZyBhdWRpbyBhbmQvb3IgdmlkZW8gZGF0YS4gfFxuICogfCB1bm11dGUgICAgICAgICAgfCBNdXRlRXZlbnQgICAgICAgIHwgUHVibGljYXRpb24gaXMgdW5tdXRlZC4gUmVtb3RlIHNpZGUgY29udGludWVkIHNlbmRpbmcgYXVkaW8gYW5kL29yIHZpZGVvIGRhdGEuIHxcbiAqXG4gKiBAZXh0ZW5kcyBPd3QuQmFzZS5FdmVudERpc3BhdGNoZXJcbiAqIEBoaWRlY29uc3RydWN0b3JcbiAqL1xuZXhwb3J0IGNsYXNzIFN1YnNjcmlwdGlvbiBleHRlbmRzIEV2ZW50RGlzcGF0Y2hlciB7XG4gIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jXG4gIGNvbnN0cnVjdG9yKFxuICAgICAgaWQsIHN0cmVhbSwgdHJhbnNwb3J0LCBzdG9wLCBnZXRTdGF0cywgbXV0ZSwgdW5tdXRlLCBhcHBseU9wdGlvbnMpIHtcbiAgICBzdXBlcigpO1xuICAgIGlmICghaWQpIHtcbiAgICAgIHRocm93IG5ldyBUeXBlRXJyb3IoJ0lEIGNhbm5vdCBiZSBudWxsIG9yIHVuZGVmaW5lZC4nKTtcbiAgICB9XG4gICAgLyoqXG4gICAgICogQG1lbWJlciB7c3RyaW5nfSBpZFxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5TdWJzY3JpcHRpb25cbiAgICAgKi9cbiAgICBPYmplY3QuZGVmaW5lUHJvcGVydHkodGhpcywgJ2lkJywge1xuICAgICAgY29uZmlndXJhYmxlOiBmYWxzZSxcbiAgICAgIHdyaXRhYmxlOiBmYWxzZSxcbiAgICAgIHZhbHVlOiBpZCxcbiAgICB9KTtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHtNZWRpYVN0cmVhbSB8IEJpZGlyZWN0aW9uYWxTdHJlYW19IHN0cmVhbVxuICAgICAqIEBpbnN0YW5jZVxuICAgICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5TdWJzY3JpcHRpb25cbiAgICAgKi9cbiAgICBPYmplY3QuZGVmaW5lUHJvcGVydHkodGhpcywgJ3N0cmVhbScsIHtcbiAgICAgIGNvbmZpZ3VyYWJsZTogZmFsc2UsXG4gICAgICAvLyBUT0RPOiBJdCBzaG91bGQgYmUgYSByZWFkb25seSBwcm9wZXJ0eSwgYnV0IGN1cnJlbnQgaW1wbGVtZW50YXRpb25cbiAgICAgIC8vIGNyZWF0ZXMgU3Vic2NyaXB0aW9uIGFmdGVyIHJlY2VpdmluZyAncmVhZHknIGZyb20gc2VydmVyLiBBdCB0aGlzIHRpbWUsXG4gICAgICAvLyBNZWRpYVN0cmVhbSBtYXkgbm90IGJlIGF2YWlsYWJsZS5cbiAgICAgIHdyaXRhYmxlOiB0cnVlLFxuICAgICAgdmFsdWU6IHN0cmVhbSxcbiAgICB9KTtcbiAgICAvKipcbiAgICAgKiBAbWVtYmVyIHtPd3QuQmFzZS5UcmFuc3BvcnRTZXR0aW5nc30gdHJhbnNwb3J0XG4gICAgICogQGluc3RhbmNlXG4gICAgICogQHJlYWRvbmx5XG4gICAgICogQGRlc2MgVHJhbnNwb3J0IHNldHRpbmdzIGZvciB0aGUgc3Vic2NyaXB0aW9uLlxuICAgICAqIEBtZW1iZXJvZiBPd3QuQmFzZS5TdWJzY3JpcHRpb25cbiAgICAgKi9cbiAgICBPYmplY3QuZGVmaW5lUHJvcGVydHkodGhpcywgJ3RyYW5zcG9ydCcsIHtcbiAgICAgIGNvbmZpZ3VyYWJsZTogZmFsc2UsXG4gICAgICB3cml0YWJsZTogZmFsc2UsXG4gICAgICB2YWx1ZTogdHJhbnNwb3J0LFxuICAgIH0pO1xuICAgIC8qKlxuICAgICAqIEBmdW5jdGlvbiBzdG9wXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQGRlc2MgU3RvcCBjZXJ0YWluIHN1YnNjcmlwdGlvbi4gT25jZSBhIHN1YnNjcmlwdGlvbiBpcyBzdG9wcGVkLCBpdCBjYW5ub3QgYmUgcmVjb3ZlcmVkLlxuICAgICAqIEBtZW1iZXJvZiBPd3QuQ29uZmVyZW5jZS5TdWJzY3JpcHRpb25cbiAgICAgKiBAcmV0dXJucyB7dW5kZWZpbmVkfVxuICAgICAqL1xuICAgIHRoaXMuc3RvcCA9IHN0b3A7XG4gICAgLyoqXG4gICAgICogQGZ1bmN0aW9uIGdldFN0YXRzXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQGRlc2MgR2V0IHN0YXRzIG9mIHVuZGVybHlpbmcgUGVlckNvbm5lY3Rpb24uXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlN1YnNjcmlwdGlvblxuICAgICAqIEByZXR1cm5zIHtQcm9taXNlPFJUQ1N0YXRzUmVwb3J0LCBFcnJvcj59XG4gICAgICovXG4gICAgdGhpcy5nZXRTdGF0cyA9IGdldFN0YXRzO1xuICAgIC8qKlxuICAgICAqIEBmdW5jdGlvbiBtdXRlXG4gICAgICogQGluc3RhbmNlXG4gICAgICogQGRlc2MgU3RvcCByZWV2aW5nIGRhdGEgZnJvbSByZW1vdGUgZW5kcG9pbnQuXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlN1YnNjcmlwdGlvblxuICAgICAqIEBwYXJhbSB7T3d0LkJhc2UuVHJhY2tLaW5kIH0ga2luZCBLaW5kIG9mIHRyYWNrcyB0byBiZSBtdXRlZC5cbiAgICAgKiBAcmV0dXJucyB7UHJvbWlzZTx1bmRlZmluZWQsIEVycm9yPn1cbiAgICAgKi9cbiAgICB0aGlzLm11dGUgPSBtdXRlO1xuICAgIC8qKlxuICAgICAqIEBmdW5jdGlvbiB1bm11dGVcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAZGVzYyBDb250aW51ZSByZWV2aW5nIGRhdGEgZnJvbSByZW1vdGUgZW5kcG9pbnQuXG4gICAgICogQG1lbWJlcm9mIE93dC5Db25mZXJlbmNlLlN1YnNjcmlwdGlvblxuICAgICAqIEBwYXJhbSB7T3d0LkJhc2UuVHJhY2tLaW5kIH0ga2luZCBLaW5kIG9mIHRyYWNrcyB0byBiZSB1bm11dGVkLlxuICAgICAqIEByZXR1cm5zIHtQcm9taXNlPHVuZGVmaW5lZCwgRXJyb3I+fVxuICAgICAqL1xuICAgIHRoaXMudW5tdXRlID0gdW5tdXRlO1xuICAgIC8qKlxuICAgICAqIEBmdW5jdGlvbiBhcHBseU9wdGlvbnNcbiAgICAgKiBAaW5zdGFuY2VcbiAgICAgKiBAZGVzYyBVcGRhdGUgc3Vic2NyaXB0aW9uIHdpdGggZ2l2ZW4gb3B0aW9ucy5cbiAgICAgKiBAbWVtYmVyb2YgT3d0LkNvbmZlcmVuY2UuU3Vic2NyaXB0aW9uXG4gICAgICogQHBhcmFtIHtPd3QuQ29uZmVyZW5jZS5TdWJzY3JpcHRpb25VcGRhdGVPcHRpb25zIH0gb3B0aW9ucyBTdWJzY3JpcHRpb24gdXBkYXRlIG9wdGlvbnMuXG4gICAgICogQHJldHVybnMge1Byb21pc2U8dW5kZWZpbmVkLCBFcnJvcj59XG4gICAgICovXG4gICAgdGhpcy5hcHBseU9wdGlvbnMgPSBhcHBseU9wdGlvbnM7XG5cbiAgICAvLyBUcmFjayBpcyBub3QgZGVmaW5lZCBpbiBzZXJ2ZXIgcHJvdG9jb2wuIFNvIHRoZXNlIElEcyBhcmUgZXF1YWwgdG8gdGhlaXJcbiAgICAvLyBzdHJlYW0ncyBJRCBhdCB0aGlzIHRpbWUuXG4gICAgdGhpcy5fYXVkaW9UcmFja0lkID0gdW5kZWZpbmVkO1xuICAgIHRoaXMuX3ZpZGVvVHJhY2tJZCA9IHVuZGVmaW5lZDtcbiAgfVxufVxuIiwiLy8gQ29weXJpZ2h0IChDKSA8MjAxOD4gSW50ZWwgQ29ycG9yYXRpb25cbi8vXG4vLyBTUERYLUxpY2Vuc2UtSWRlbnRpZmllcjogQXBhY2hlLTIuMFxuXG4ndXNlIHN0cmljdCc7XG5cbmltcG9ydCAqIGFzIGJhc2UgZnJvbSAnLi9iYXNlL2V4cG9ydC5qcyc7XG5pbXBvcnQgKiBhcyBwMnAgZnJvbSAnLi9wMnAvZXhwb3J0LmpzJztcbmltcG9ydCAqIGFzIGNvbmZlcmVuY2UgZnJvbSAnLi9jb25mZXJlbmNlL2V4cG9ydC5qcyc7XG5cbi8qKlxuICogQmFzZSBvYmplY3RzIGZvciBib3RoIFAyUCBhbmQgY29uZmVyZW5jZS5cbiAqIEBuYW1lc3BhY2UgT3d0LkJhc2VcbiAqL1xuZXhwb3J0IGNvbnN0IEJhc2UgPSBiYXNlO1xuXG4vKipcbiAqIFAyUCBXZWJSVEMgY29ubmVjdGlvbnMuXG4gKiBAbmFtZXNwYWNlIE93dC5QMlBcbiAqL1xuZXhwb3J0IGNvbnN0IFAyUCA9IHAycDtcblxuLyoqXG4gKiBXZWJSVEMgY29ubmVjdGlvbnMgd2l0aCBjb25mZXJlbmNlIHNlcnZlci5cbiAqIEBuYW1lc3BhY2UgT3d0LkNvbmZlcmVuY2VcbiAqL1xuZXhwb3J0IGNvbnN0IENvbmZlcmVuY2UgPSBjb25mZXJlbmNlO1xuIiwiLy8gQ29weXJpZ2h0IChDKSA8MjAxOD4gSW50ZWwgQ29ycG9yYXRpb25cbi8vXG4vLyBTUERYLUxpY2Vuc2UtSWRlbnRpZmllcjogQXBhY2hlLTIuMFxuXG4ndXNlIHN0cmljdCc7XG5cbmV4cG9ydCBjb25zdCBlcnJvcnMgPSB7XG4gIC8vIDIxMDAtMjk5OSBmb3IgUDJQIGVycm9yc1xuICAvLyAyMTAwLTIxOTkgZm9yIGNvbm5lY3Rpb24gZXJyb3JzXG4gIC8vIDIxMDAtMjEwOSBmb3Igc2VydmVyIGVycm9yc1xuICBQMlBfQ09OTl9TRVJWRVJfVU5LTk9XTjoge1xuICAgIGNvZGU6IDIxMDAsXG4gICAgbWVzc2FnZTogJ1NlcnZlciB1bmtub3duIGVycm9yLicsXG4gIH0sXG4gIFAyUF9DT05OX1NFUlZFUl9VTkFWQUlMQUJMRToge1xuICAgIGNvZGU6IDIxMDEsXG4gICAgbWVzc2FnZTogJ1NlcnZlciBpcyB1bmF2YWxpYWJsZS4nLFxuICB9LFxuICBQMlBfQ09OTl9TRVJWRVJfQlVTWToge1xuICAgIGNvZGU6IDIxMDIsXG4gICAgbWVzc2FnZTogJ1NlcnZlciBpcyB0b28gYnVzeS4nLFxuICB9LFxuICBQMlBfQ09OTl9TRVJWRVJfTk9UX1NVUFBPUlRFRDoge1xuICAgIGNvZGU6IDIxMDMsXG4gICAgbWVzc2FnZTogJ01ldGhvZCBoYXMgbm90IGJlZW4gc3VwcG9ydGVkIGJ5IHNlcnZlci4nLFxuICB9LFxuICAvLyAyMTEwLTIxMTkgZm9yIGNsaWVudCBlcnJvcnNcbiAgUDJQX0NPTk5fQ0xJRU5UX1VOS05PV046IHtcbiAgICBjb2RlOiAyMTEwLFxuICAgIG1lc3NhZ2U6ICdDbGllbnQgdW5rbm93biBlcnJvci4nLFxuICB9LFxuICBQMlBfQ09OTl9DTElFTlRfTk9UX0lOSVRJQUxJWkVEOiB7XG4gICAgY29kZTogMjExMSxcbiAgICBtZXNzYWdlOiAnQ29ubmVjdGlvbiBpcyBub3QgaW5pdGlhbGl6ZWQuJyxcbiAgfSxcbiAgLy8gMjEyMC0yMTI5IGZvciBhdXRoZW50aWNhdGlvbiBlcnJvcnNcbiAgUDJQX0NPTk5fQVVUSF9VTktOT1dOOiB7XG4gICAgY29kZTogMjEyMCxcbiAgICBtZXNzYWdlOiAnQXV0aGVudGljYXRpb24gdW5rbm93biBlcnJvci4nLFxuICB9LFxuICBQMlBfQ09OTl9BVVRIX0ZBSUxFRDoge1xuICAgIGNvZGU6IDIxMjEsXG4gICAgbWVzc2FnZTogJ1dyb25nIHVzZXJuYW1lIG9yIHRva2VuLicsXG4gIH0sXG4gIC8vIDIyMDAtMjI5OSBmb3IgbWVzc2FnZSB0cmFuc3BvcnQgZXJyb3JzXG4gIFAyUF9NRVNTQUdJTkdfVEFSR0VUX1VOUkVBQ0hBQkxFOiB7XG4gICAgY29kZTogMjIwMSxcbiAgICBtZXNzYWdlOiAnUmVtb3RlIHVzZXIgY2Fubm90IGJlIHJlYWNoZWQuJyxcbiAgfSxcbiAgUDJQX0NMSUVOVF9ERU5JRUQ6IHtcbiAgICBjb2RlOiAyMjAyLFxuICAgIG1lc3NhZ2U6ICdVc2VyIGlzIGRlbmllZC4nLFxuICB9LFxuICAvLyAyMzAxLTIzOTkgZm9yIGNoYXQgcm9vbSBlcnJvcnNcbiAgLy8gMjQwMS0yNDk5IGZvciBjbGllbnQgZXJyb3JzXG4gIFAyUF9DTElFTlRfVU5LTk9XTjoge1xuICAgIGNvZGU6IDI0MDAsXG4gICAgbWVzc2FnZTogJ1Vua25vd24gZXJyb3JzLicsXG4gIH0sXG4gIFAyUF9DTElFTlRfVU5TVVBQT1JURURfTUVUSE9EOiB7XG4gICAgY29kZTogMjQwMSxcbiAgICBtZXNzYWdlOiAnVGhpcyBtZXRob2QgaXMgdW5zdXBwb3J0ZWQgaW4gY3VycmVudCBicm93c2VyLicsXG4gIH0sXG4gIFAyUF9DTElFTlRfSUxMRUdBTF9BUkdVTUVOVDoge1xuICAgIGNvZGU6IDI0MDIsXG4gICAgbWVzc2FnZTogJ0lsbGVnYWwgYXJndW1lbnQuJyxcbiAgfSxcbiAgUDJQX0NMSUVOVF9JTlZBTElEX1NUQVRFOiB7XG4gICAgY29kZTogMjQwMyxcbiAgICBtZXNzYWdlOiAnSW52YWxpZCBwZWVyIHN0YXRlLicsXG4gIH0sXG4gIFAyUF9DTElFTlRfTk9UX0FMTE9XRUQ6IHtcbiAgICBjb2RlOiAyNDA0LFxuICAgIG1lc3NhZ2U6ICdSZW1vdGUgdXNlciBpcyBub3QgYWxsb3dlZC4nLFxuICB9LFxuICAvLyAyNTAxLTI1OTkgZm9yIFdlYlJUQyBlcnJvcy5cbiAgUDJQX1dFQlJUQ19VTktOT1dOOiB7XG4gICAgY29kZTogMjUwMCxcbiAgICBtZXNzYWdlOiAnV2ViUlRDIGVycm9yLicsXG4gIH0sXG4gIFAyUF9XRUJSVENfU0RQOiB7XG4gICAgY29kZTogMjUwMixcbiAgICBtZXNzYWdlOiAnU0RQIGVycm9yLicsXG4gIH0sXG59O1xuXG4vKipcbiAqIEBmdW5jdGlvbiBnZXRFcnJvckJ5Q29kZVxuICogQGRlc2MgR2V0IGVycm9yIG9iamVjdCBieSBlcnJvciBjb2RlLlxuICogQHBhcmFtIHtzdHJpbmd9IGVycm9yQ29kZSBFcnJvciBjb2RlLlxuICogQHJldHVybiB7T3d0LlAyUC5FcnJvcn0gRXJyb3Igb2JqZWN0XG4gKiBAcHJpdmF0ZVxuICovXG5leHBvcnQgZnVuY3Rpb24gZ2V0RXJyb3JCeUNvZGUoZXJyb3JDb2RlKSB7XG4gIGNvbnN0IGNvZGVFcnJvck1hcCA9IHtcbiAgICAyMTAwOiBlcnJvcnMuUDJQX0NPTk5fU0VSVkVSX1VOS05PV04sXG4gICAgMjEwMTogZXJyb3JzLlAyUF9DT05OX1NFUlZFUl9VTkFWQUlMQUJMRSxcbiAgICAyMTAyOiBlcnJvcnMuUDJQX0NPTk5fU0VSVkVSX0JVU1ksXG4gICAgMjEwMzogZXJyb3JzLlAyUF9DT05OX1NFUlZFUl9OT1RfU1VQUE9SVEVELFxuICAgIDIxMTA6IGVycm9ycy5QMlBfQ09OTl9DTElFTlRfVU5LTk9XTixcbiAgICAyMTExOiBlcnJvcnMuUDJQX0NPTk5fQ0xJRU5UX05PVF9JTklUSUFMSVpFRCxcbiAgICAyMTIwOiBlcnJvcnMuUDJQX0NPTk5fQVVUSF9VTktOT1dOLFxuICAgIDIxMjE6IGVycm9ycy5QMlBfQ09OTl9BVVRIX0ZBSUxFRCxcbiAgICAyMjAxOiBlcnJvcnMuUDJQX01FU1NBR0lOR19UQVJHRVRfVU5SRUFDSEFCTEUsXG4gICAgMjQwMDogZXJyb3JzLlAyUF9DTElFTlRfVU5LTk9XTixcbiAgICAyNDAxOiBlcnJvcnMuUDJQX0NMSUVOVF9VTlNVUFBPUlRFRF9NRVRIT0QsXG4gICAgMjQwMjogZXJyb3JzLlAyUF9DTElFTlRfSUxMRUdBTF9BUkdVTUVOVCxcbiAgICAyNDAzOiBlcnJvcnMuUDJQX0NMSUVOVF9JTlZBTElEX1NUQVRFLFxuICAgIDI0MDQ6IGVycm9ycy5QMlBfQ0xJRU5UX05PVF9BTExPV0VELFxuICAgIDI1MDA6IGVycm9ycy5QMlBfV0VCUlRDX1VOS05PV04sXG4gICAgMjUwMTogZXJyb3JzLlAyUF9XRUJSVENfU0RQLFxuICB9O1xuICByZXR1cm4gY29kZUVycm9yTWFwW2Vycm9yQ29kZV07XG59XG5cbi8qKlxuICogQGNsYXNzIFAyUEVycm9yXG4gKiBAY2xhc3NEZXNjIFRoZSBQMlBFcnJvciBvYmplY3QgcmVwcmVzZW50cyBhbiBlcnJvciBpbiBQMlAgbW9kZS5cbiAqIEBtZW1iZXJPZiBPd3QuUDJQXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKi9cbmV4cG9ydCBjbGFzcyBQMlBFcnJvciBleHRlbmRzIEVycm9yIHtcbiAgLy8gZXNsaW50LWRpc2FibGUtbmV4dC1saW5lIHJlcXVpcmUtanNkb2NcbiAgY29uc3RydWN0b3IoZXJyb3IsIG1lc3NhZ2UpIHtcbiAgICBzdXBlcihtZXNzYWdlKTtcbiAgICBpZiAodHlwZW9mIGVycm9yID09PSAnbnVtYmVyJykge1xuICAgICAgdGhpcy5jb2RlID0gZXJyb3I7XG4gICAgfSBlbHNlIHtcbiAgICAgIHRoaXMuY29kZSA9IGVycm9yLmNvZGU7XG4gICAgfVxuICB9XG59XG4iLCIvLyBDb3B5cmlnaHQgKEMpIDwyMDE4PiBJbnRlbCBDb3Jwb3JhdGlvblxuLy9cbi8vIFNQRFgtTGljZW5zZS1JZGVudGlmaWVyOiBBcGFjaGUtMi4wXG5cbid1c2Ugc3RyaWN0JztcblxuZXhwb3J0IHtkZWZhdWx0IGFzIFAyUENsaWVudH0gZnJvbSAnLi9wMnBjbGllbnQuanMnO1xuZXhwb3J0IHtQMlBFcnJvcn0gZnJvbSAnLi9lcnJvci5qcyc7XG4iLCIvLyBDb3B5cmlnaHQgKEMpIDwyMDE4PiBJbnRlbCBDb3Jwb3JhdGlvblxuLy9cbi8vIFNQRFgtTGljZW5zZS1JZGVudGlmaWVyOiBBcGFjaGUtMi4wXG5cbi8qIGdsb2JhbCBNYXAsIFByb21pc2UgKi9cblxuJ3VzZSBzdHJpY3QnO1xuaW1wb3J0IExvZ2dlciBmcm9tICcuLi9iYXNlL2xvZ2dlci5qcyc7XG5pbXBvcnQge0V2ZW50RGlzcGF0Y2hlciwgT3d0RXZlbnR9IGZyb20gJy4uL2Jhc2UvZXZlbnQuanMnO1xuaW1wb3J0ICogYXMgRXJyb3JNb2R1bGUgZnJvbSAnLi9lcnJvci5qcyc7XG5pbXBvcnQgUDJQUGVlckNvbm5lY3Rpb25DaGFubmVsIGZyb20gJy4vcGVlcmNvbm5lY3Rpb24tY2hhbm5lbC5qcyc7XG5cbmNvbnN0IENvbm5lY3Rpb25TdGF0ZSA9IHtcbiAgUkVBRFk6IDEsXG4gIENPTk5FQ1RJTkc6IDIsXG4gIENPTk5FQ1RFRDogMyxcbn07XG5cbi8qIGVzbGludC1kaXNhYmxlIG5vLXVudXNlZC12YXJzICovXG4vKipcbiAqIEBjbGFzcyBQMlBDbGllbnRDb25maWd1cmF0aW9uXG4gKiBAY2xhc3NEZXNjIENvbmZpZ3VyYXRpb24gZm9yIFAyUENsaWVudC5cbiAqIEBtZW1iZXJPZiBPd3QuUDJQXG4gKiBAaGlkZWNvbnN0cnVjdG9yXG4gKi9cbmNvbnN0IFAyUENsaWVudENvbmZpZ3VyYXRpb24gPSBmdW5jdGlvbigpIHtcbiAgLyoqXG4gICAqIEBtZW1iZXIgez9BcnJheTxPd3QuQmFzZS5BdWRpb0VuY29kaW5nUGFyYW1ldGVycz59IGF1ZGlvRW5jb2RpbmdcbiAgICogQGluc3RhbmNlXG4gICAqIEBkZXNjIEVuY29kaW5nIHBhcmFtZXRlcnMgZm9yIHB1Ymxpc2hpbmcgYXVkaW8gdHJhY2tzLlxuICAgKiBAbWVtYmVyb2YgT3d0LlAyUC5QMlBDbGllbnRDb25maWd1cmF0aW9uXG4gICAqL1xuICB0aGlzLmF1ZGlvRW5jb2RpbmcgPSB1bmRlZmluZWQ7XG4gIC8qKlxuICAgKiBAbWVtYmVyIHs/QXJyYXk8T3d0LkJhc2UuVmlkZW9FbmNvZGluZ1BhcmFtZXRlcnM+fSB2aWRlb0VuY29kaW5nXG4gICAqIEBpbnN0YW5jZVxuICAgKiBAZGVzYyBFbmNvZGluZyBwYXJhbWV0ZXJzIGZvciBwdWJsaXNoaW5nIHZpZGVvIHRyYWNrcy5cbiAgICogQG1lbWJlcm9mIE93dC5QMlAuUDJQQ2xpZW50Q29uZmlndXJhdGlvblxuICAgKi9cbiAgdGhpcy52aWRlb0VuY29kaW5nID0gdW5kZWZpbmVkO1xuICAvKipcbiAgICogQG1lbWJlciB7P1JUQ0NvbmZpZ3VyYXRpb259IHJ0Y0NvbmZpZ3VyYXRpb25cbiAgICogQGluc3RhbmNlXG4gICAqIEBtZW1iZXJvZiBPd3QuUDJQLlAyUENsaWVudENvbmZpZ3VyYXRpb25cbiAgICogQGRlc2MgSXQgd2lsbCBiZSB1c2VkIGZvciBjcmVhdGluZyBQZWVyQ29ubmVjdGlvbi5cbiAgICogQHNlZSB7QGxpbmsgaHR0cHM6Ly93d3cudzMub3JnL1RSL3dlYnJ0Yy8jcnRjY29uZmlndXJhdGlvbi1kaWN0aW9uYXJ5fFJUQ0NvbmZpZ3VyYXRpb24gRGljdGlvbmFyeSBvZiBXZWJSVEMgMS4wfS5cbiAgICogQGV4YW1wbGVcbiAgICogLy8gRm9sbG93aW5nIG9iamVjdCBjYW4gYmUgc2V0IHRvIHAycENsaWVudENvbmZpZ3VyYXRpb24ucnRjQ29uZmlndXJhdGlvbi5cbiAgICoge1xuICAgKiAgIGljZVNlcnZlcnM6IFt7XG4gICAqICAgICAgdXJsczogXCJzdHVuOmV4YW1wbGUuY29tOjM0NzhcIlxuICAgKiAgIH0sIHtcbiAgICogICAgIHVybHM6IFtcbiAgICogICAgICAgXCJ0dXJuOmV4YW1wbGUuY29tOjM0Nzg/dHJhbnNwb3J0PXVkcFwiLFxuICAgKiAgICAgICBcInR1cm46ZXhhbXBsZS5jb206MzQ3OD90cmFuc3BvcnQ9dGNwXCJcbiAgICogICAgIF0sXG4gICAqICAgICAgY3JlZGVudGlhbDogXCJwYXNzd29yZFwiLFxuICAgKiAgICAgIHVzZXJuYW1lOiBcInVzZXJuYW1lXCJcbiAgICogICB9XG4gICAqIH1cbiAgICovXG4gIHRoaXMucnRjQ29uZmlndXJhdGlvbiA9IHVuZGVmaW5lZDtcbn07XG4vKiBlc2xpbnQtZW5hYmxlIG5vLXVudXNlZC12YXJzICovXG5cbi8qKlxuICogQGNsYXNzIFAyUENsaWVudFxuICogQGNsYXNzRGVzYyBUaGUgUDJQQ2xpZW50IGhhbmRsZXMgUGVlckNvbm5lY3Rpb25zIGJldHdlZW4gZGlmZmVyZW50IGNsaWVudHMuXG4gKiBFdmVudHM6XG4gKlxuICogfCBFdmVudCBOYW1lICAgICAgICAgICAgfCBBcmd1bWVudCBUeXBlICAgIHwgRmlyZWQgd2hlbiAgICAgICB8XG4gKiB8IC0tLS0tLS0tLS0tLS0tLS0tLS0tLSB8IC0tLS0tLS0tLS0tLS0tLS0gfCAtLS0tLS0tLS0tLS0tLS0tIHxcbiAqIHwgc3RyZWFtYWRkZWQgICAgICAgICAgIHwgU3RyZWFtRXZlbnQgICAgICB8IEEgbmV3IHN0cmVhbSBpcyBzZW50IGZyb20gcmVtb3RlIGVuZHBvaW50LiB8XG4gKiB8IG1lc3NhZ2VyZWNlaXZlZCAgICAgICB8IE1lc3NhZ2VFdmVudCAgICAgfCBBIG5ldyBtZXNzYWdlIGlzIHJlY2VpdmVkLiB8XG4gKiB8IHNlcnZlcmRpc2Nvbm5lY3RlZCAgICB8IE93dEV2ZW50ICAgICAgICAgfCBEaXNjb25uZWN0ZWQgZnJvbSBzaWduYWxpbmcgc2VydmVyLiB8XG4gKlxuICogQG1lbWJlcm9mIE93dC5QMlBcbiAqIEBleHRlbmRzIE93dC5CYXNlLkV2ZW50RGlzcGF0Y2hlclxuICogQGNvbnN0cnVjdG9yXG4gKiBAcGFyYW0gez9Pd3QuUDJQLlAyUENsaWVudENvbmZpZ3VyYXRpb24gfSBjb25maWd1cmF0aW9uIENvbmZpZ3VyYXRpb24gZm9yIE93dC5QMlAuUDJQQ2xpZW50LlxuICogQHBhcmFtIHtPYmplY3R9IHNpZ25hbGluZ0NoYW5uZWwgQSBjaGFubmVsIGZvciBzZW5kaW5nIGFuZCByZWNlaXZpbmcgc2lnbmFsaW5nIG1lc3NhZ2VzLlxuICovXG5jb25zdCBQMlBDbGllbnQgPSBmdW5jdGlvbihjb25maWd1cmF0aW9uLCBzaWduYWxpbmdDaGFubmVsKSB7XG4gIE9iamVjdC5zZXRQcm90b3R5cGVPZih0aGlzLCBuZXcgRXZlbnREaXNwYXRjaGVyKCkpO1xuICBjb25zdCBjb25maWcgPSBjb25maWd1cmF0aW9uO1xuICBjb25zdCBzaWduYWxpbmcgPSBzaWduYWxpbmdDaGFubmVsO1xuICBjb25zdCBjaGFubmVscyA9IG5ldyBNYXAoKTsgLy8gTWFwIG9mIFBlZXJDb25uZWN0aW9uQ2hhbm5lbHMuXG4gIGNvbnN0IGNvbm5lY3Rpb25JZHMgPSBuZXcgTWFwKCk7IC8vIEtleSBpcyByZW1vdGUgdXNlciBJRCwgdmFsdWUgaXMgY3VycmVudCBzZXNzaW9uIElELlxuICBjb25zdCBzZWxmID0gdGhpcztcbiAgbGV0IHN0YXRlID0gQ29ubmVjdGlvblN0YXRlLlJFQURZO1xuICBsZXQgbXlJZDtcblxuICBzaWduYWxpbmcub25NZXNzYWdlID0gZnVuY3Rpb24ob3JpZ2luLCBtZXNzYWdlKSB7XG4gICAgTG9nZ2VyLmRlYnVnKCdSZWNlaXZlZCBzaWduYWxpbmcgbWVzc2FnZSBmcm9tICcgKyBvcmlnaW4gKyAnOiAnICsgbWVzc2FnZSk7XG4gICAgY29uc3QgZGF0YSA9IEpTT04ucGFyc2UobWVzc2FnZSk7XG4gICAgY29uc3QgY29ubmVjdGlvbklkID0gZGF0YS5jb25uZWN0aW9uSWQ7XG4gICAgaWYgKHNlbGYuYWxsb3dlZFJlbW90ZUlkcy5pbmRleE9mKG9yaWdpbikgPCAwKSB7XG4gICAgICBzZW5kU2lnbmFsaW5nTWVzc2FnZShcbiAgICAgICAgICBvcmlnaW4sIGRhdGEuY29ubmVjdGlvbklkLCAnY2hhdC1jbG9zZWQnLFxuICAgICAgICAgIEVycm9yTW9kdWxlLmVycm9ycy5QMlBfQ0xJRU5UX0RFTklFRCk7XG4gICAgICByZXR1cm47XG4gICAgfVxuICAgIGlmIChjb25uZWN0aW9uSWRzLmhhcyhvcmlnaW4pICYmXG4gICAgICAgIGNvbm5lY3Rpb25JZHMuZ2V0KG9yaWdpbikgIT09IGNvbm5lY3Rpb25JZCAmJiAhaXNQb2xpdGVQZWVyKG9yaWdpbikpIHtcbiAgICAgIExvZ2dlci53YXJuaW5nKFxuICAgICAgICAgIC8vIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSBtYXgtbGVuXG4gICAgICAgICAgJ0NvbGxpc2lvbiBkZXRlY3RlZCwgaWdub3JlIHRoaXMgbWVzc2FnZSBiZWNhdXNlIGN1cnJlbnQgZW5kcG9pbnQgaXMgaW1wb2xpdGUgcGVlci4nKTtcbiAgICAgIHJldHVybjtcbiAgICB9XG4gICAgaWYgKGRhdGEudHlwZSA9PT0gJ2NoYXQtY2xvc2VkJykge1xuICAgICAgaWYgKGNoYW5uZWxzLmhhcyhvcmlnaW4pKSB7XG4gICAgICAgIGdldE9yQ3JlYXRlQ2hhbm5lbChvcmlnaW4sIGNvbm5lY3Rpb25JZCkub25NZXNzYWdlKGRhdGEpO1xuICAgICAgICBjaGFubmVscy5kZWxldGUob3JpZ2luKTtcbiAgICAgIH1cbiAgICAgIHJldHVybjtcbiAgICB9XG4gICAgZ2V0T3JDcmVhdGVDaGFubmVsKG9yaWdpbiwgY29ubmVjdGlvbklkKS5vbk1lc3NhZ2UoZGF0YSk7XG4gIH07XG5cbiAgc2lnbmFsaW5nLm9uU2VydmVyRGlzY29ubmVjdGVkID0gZnVuY3Rpb24oKSB7XG4gICAgc3RhdGUgPSBDb25uZWN0aW9uU3RhdGUuUkVBRFk7XG4gICAgc2VsZi5kaXNwYXRjaEV2ZW50KG5ldyBPd3RFdmVudCgnc2VydmVyZGlzY29ubmVjdGVkJykpO1xuICB9O1xuXG4gIC8qKlxuICAgKiBAbWVtYmVyIHthcnJheX0gYWxsb3dlZFJlbW90ZUlkc1xuICAgKiBAbWVtYmVyb2YgT3d0LlAyUC5QMlBDbGllbnRcbiAgICogQGluc3RhbmNlXG4gICAqIEBkZXNjIE9ubHkgYWxsb3dlZCByZW1vdGUgZW5kcG9pbnQgSURzIGFyZSBhYmxlIHRvIHB1Ymxpc2ggc3RyZWFtIG9yIHNlbmQgbWVzc2FnZSB0byBjdXJyZW50IGVuZHBvaW50LiBSZW1vdmluZyBhbiBJRCBmcm9tIGFsbG93ZWRSZW1vdGVJZHMgZG9lcyBzdG9wIGV4aXN0aW5nIGNvbm5lY3Rpb24gd2l0aCBjZXJ0YWluIGVuZHBvaW50LiBQbGVhc2UgY2FsbCBzdG9wIHRvIHN0b3AgdGhlIFBlZXJDb25uZWN0aW9uLlxuICAgKi9cbiAgdGhpcy5hbGxvd2VkUmVtb3RlSWRzPVtdO1xuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gY29ubmVjdFxuICAgKiBAaW5zdGFuY2VcbiAgICogQGRlc2MgQ29ubmVjdCB0byBzaWduYWxpbmcgc2VydmVyLiBTaW5jZSBzaWduYWxpbmcgY2FuIGJlIGN1c3RvbWl6ZWQsIHRoaXMgbWV0aG9kIGRvZXMgbm90IGRlZmluZSBob3cgYSB0b2tlbiBsb29rcyBsaWtlLiBTREsgcGFzc2VzIHRva2VuIHRvIHNpZ25hbGluZyBjaGFubmVsIHdpdGhvdXQgY2hhbmdlcy5cbiAgICogQG1lbWJlcm9mIE93dC5QMlAuUDJQQ2xpZW50XG4gICAqIEBwYXJhbSB7c3RyaW5nfSB0b2tlbiBBIHRva2VuIGZvciBjb25uZWN0aW5nIHRvIHNpZ25hbGluZyBzZXJ2ZXIuIFRoZSBmb3JtYXQgb2YgdGhpcyB0b2tlbiBkZXBlbmRzIG9uIHNpZ25hbGluZyBzZXJ2ZXIncyByZXF1aXJlbWVudC5cbiAgICogQHJldHVybiB7UHJvbWlzZTxvYmplY3QsIEVycm9yPn0gSXQgcmV0dXJucyBhIHByb21pc2UgcmVzb2x2ZWQgd2l0aCBhbiBvYmplY3QgcmV0dXJuZWQgYnkgc2lnbmFsaW5nIGNoYW5uZWwgb25jZSBzaWduYWxpbmcgY2hhbm5lbCByZXBvcnRzIGNvbm5lY3Rpb24gaGFzIGJlZW4gZXN0YWJsaXNoZWQuXG4gICAqL1xuICB0aGlzLmNvbm5lY3QgPSBmdW5jdGlvbih0b2tlbikge1xuICAgIGlmIChzdGF0ZSA9PT0gQ29ubmVjdGlvblN0YXRlLlJFQURZKSB7XG4gICAgICBzdGF0ZSA9IENvbm5lY3Rpb25TdGF0ZS5DT05ORUNUSU5HO1xuICAgIH0gZWxzZSB7XG4gICAgICBMb2dnZXIud2FybmluZygnSW52YWxpZCBjb25uZWN0aW9uIHN0YXRlOiAnICsgc3RhdGUpO1xuICAgICAgcmV0dXJuIFByb21pc2UucmVqZWN0KG5ldyBFcnJvck1vZHVsZS5QMlBFcnJvcihcbiAgICAgICAgICBFcnJvck1vZHVsZS5lcnJvcnMuUDJQX0NMSUVOVF9JTlZBTElEX1NUQVRFKSk7XG4gICAgfVxuICAgIHJldHVybiBuZXcgUHJvbWlzZSgocmVzb2x2ZSwgcmVqZWN0KSA9PiB7XG4gICAgICBzaWduYWxpbmcuY29ubmVjdCh0b2tlbikudGhlbigoaWQpID0+IHtcbiAgICAgICAgbXlJZCA9IGlkO1xuICAgICAgICBzdGF0ZSA9IENvbm5lY3Rpb25TdGF0ZS5DT05ORUNURUQ7XG4gICAgICAgIHJlc29sdmUobXlJZCk7XG4gICAgICB9LCAoZXJyQ29kZSkgPT4ge1xuICAgICAgICByZWplY3QobmV3IEVycm9yTW9kdWxlLlAyUEVycm9yKEVycm9yTW9kdWxlLmdldEVycm9yQnlDb2RlKFxuICAgICAgICAgICAgZXJyQ29kZSkpKTtcbiAgICAgIH0pO1xuICAgIH0pO1xuICB9O1xuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gZGlzY29ubmVjdFxuICAgKiBAaW5zdGFuY2VcbiAgICogQGRlc2MgRGlzY29ubmVjdCBmcm9tIHRoZSBzaWduYWxpbmcgY2hhbm5lbC4gSXQgc3RvcHMgYWxsIGV4aXN0aW5nIHNlc3Npb25zIHdpdGggcmVtb3RlIGVuZHBvaW50cy5cbiAgICogQG1lbWJlcm9mIE93dC5QMlAuUDJQQ2xpZW50XG4gICAqL1xuICB0aGlzLmRpc2Nvbm5lY3QgPSBmdW5jdGlvbigpIHtcbiAgICBpZiAoc3RhdGUgPT0gQ29ubmVjdGlvblN0YXRlLlJFQURZKSB7XG4gICAgICByZXR1cm47XG4gICAgfVxuICAgIGNoYW5uZWxzLmZvckVhY2goKGNoYW5uZWwpID0+IHtcbiAgICAgIGNoYW5uZWwuc3RvcCgpO1xuICAgIH0pO1xuICAgIGNoYW5uZWxzLmNsZWFyKCk7XG4gICAgc2lnbmFsaW5nLmRpc2Nvbm5lY3QoKTtcbiAgfTtcblxuICAvKipcbiAgICogQGZ1bmN0aW9uIHB1Ymxpc2hcbiAgICogQGluc3RhbmNlXG4gICAqIEBkZXNjIFB1Ymxpc2ggYSBzdHJlYW0gdG8gYSByZW1vdGUgZW5kcG9pbnQuXG4gICAqIEBtZW1iZXJvZiBPd3QuUDJQLlAyUENsaWVudFxuICAgKiBAcGFyYW0ge3N0cmluZ30gcmVtb3RlSWQgUmVtb3RlIGVuZHBvaW50J3MgSUQuXG4gICAqIEBwYXJhbSB7T3d0LkJhc2UuTG9jYWxTdHJlYW19IHN0cmVhbSBBbiBPd3QuQmFzZS5Mb2NhbFN0cmVhbSB0byBiZSBwdWJsaXNoZWQuXG4gICAqIEByZXR1cm4ge1Byb21pc2U8T3d0LkJhc2UuUHVibGljYXRpb24sIEVycm9yPn0gQSBwcm9taXNlZCB0aGF0IHJlc29sdmVzIHdoZW4gcmVtb3RlIHNpZGUgcmVjZWl2ZWQgdGhlIGNlcnRhaW4gc3RyZWFtLiBIb3dldmVyLCByZW1vdGUgZW5kcG9pbnQgbWF5IG5vdCBkaXNwbGF5IHRoaXMgc3RyZWFtLCBvciBpZ25vcmUgaXQuXG4gICAqL1xuICB0aGlzLnB1Ymxpc2ggPSBmdW5jdGlvbihyZW1vdGVJZCwgc3RyZWFtKSB7XG4gICAgaWYgKHN0YXRlICE9PSBDb25uZWN0aW9uU3RhdGUuQ09OTkVDVEVEKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IEVycm9yTW9kdWxlLlAyUEVycm9yKFxuICAgICAgICAgIEVycm9yTW9kdWxlLmVycm9ycy5QMlBfQ0xJRU5UX0lOVkFMSURfU1RBVEUsXG4gICAgICAgICAgJ1AyUCBDbGllbnQgaXMgbm90IGNvbm5lY3RlZCB0byBzaWduYWxpbmcgY2hhbm5lbC4nKSk7XG4gICAgfVxuICAgIGlmICh0aGlzLmFsbG93ZWRSZW1vdGVJZHMuaW5kZXhPZihyZW1vdGVJZCkgPCAwKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IEVycm9yTW9kdWxlLlAyUEVycm9yKFxuICAgICAgICAgIEVycm9yTW9kdWxlLmVycm9ycy5QMlBfQ0xJRU5UX05PVF9BTExPV0VEKSk7XG4gICAgfVxuICAgIHJldHVybiBQcm9taXNlLnJlc29sdmUoZ2V0T3JDcmVhdGVDaGFubmVsKHJlbW90ZUlkKS5wdWJsaXNoKHN0cmVhbSkpO1xuICB9O1xuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gc2VuZFxuICAgKiBAaW5zdGFuY2VcbiAgICogQGRlc2MgU2VuZCBhIG1lc3NhZ2UgdG8gcmVtb3RlIGVuZHBvaW50LlxuICAgKiBAbWVtYmVyb2YgT3d0LlAyUC5QMlBDbGllbnRcbiAgICogQHBhcmFtIHtzdHJpbmd9IHJlbW90ZUlkIFJlbW90ZSBlbmRwb2ludCdzIElELlxuICAgKiBAcGFyYW0ge3N0cmluZ30gbWVzc2FnZSBNZXNzYWdlIHRvIGJlIHNlbnQuIEl0IHNob3VsZCBiZSBhIHN0cmluZy5cbiAgICogQHJldHVybiB7UHJvbWlzZTx1bmRlZmluZWQsIEVycm9yPn0gSXQgcmV0dXJucyBhIHByb21pc2UgcmVzb2x2ZWQgd2hlbiByZW1vdGUgZW5kcG9pbnQgcmVjZWl2ZWQgY2VydGFpbiBtZXNzYWdlLlxuICAgKi9cbiAgdGhpcy5zZW5kPWZ1bmN0aW9uKHJlbW90ZUlkLCBtZXNzYWdlKSB7XG4gICAgaWYgKHN0YXRlICE9PSBDb25uZWN0aW9uU3RhdGUuQ09OTkVDVEVEKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IEVycm9yTW9kdWxlLlAyUEVycm9yKFxuICAgICAgICAgIEVycm9yTW9kdWxlLmVycm9ycy5QMlBfQ0xJRU5UX0lOVkFMSURfU1RBVEUsXG4gICAgICAgICAgJ1AyUCBDbGllbnQgaXMgbm90IGNvbm5lY3RlZCB0byBzaWduYWxpbmcgY2hhbm5lbC4nKSk7XG4gICAgfVxuICAgIGlmICh0aGlzLmFsbG93ZWRSZW1vdGVJZHMuaW5kZXhPZihyZW1vdGVJZCkgPCAwKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IEVycm9yTW9kdWxlLlAyUEVycm9yKFxuICAgICAgICAgIEVycm9yTW9kdWxlLmVycm9ycy5QMlBfQ0xJRU5UX05PVF9BTExPV0VEKSk7XG4gICAgfVxuICAgIHJldHVybiBQcm9taXNlLnJlc29sdmUoZ2V0T3JDcmVhdGVDaGFubmVsKHJlbW90ZUlkKS5zZW5kKG1lc3NhZ2UpKTtcbiAgfTtcblxuICAvKipcbiAgICogQGZ1bmN0aW9uIHN0b3BcbiAgICogQGluc3RhbmNlXG4gICAqIEBkZXNjIENsZWFuIGFsbCByZXNvdXJjZXMgYXNzb2NpYXRlZCB3aXRoIGdpdmVuIHJlbW90ZSBlbmRwb2ludC4gSXQgbWF5IGluY2x1ZGUgUlRDUGVlckNvbm5lY3Rpb24sIFJUQ1J0cFRyYW5zY2VpdmVyIGFuZCBSVENEYXRhQ2hhbm5lbC4gSXQgc3RpbGwgcG9zc2libGUgdG8gcHVibGlzaCBhIHN0cmVhbSwgb3Igc2VuZCBhIG1lc3NhZ2UgdG8gZ2l2ZW4gcmVtb3RlIGVuZHBvaW50IGFmdGVyIHN0b3AuXG4gICAqIEBtZW1iZXJvZiBPd3QuUDJQLlAyUENsaWVudFxuICAgKiBAcGFyYW0ge3N0cmluZ30gcmVtb3RlSWQgUmVtb3RlIGVuZHBvaW50J3MgSUQuXG4gICAqIEByZXR1cm4ge3VuZGVmaW5lZH1cbiAgICovXG4gIHRoaXMuc3RvcCA9IGZ1bmN0aW9uKHJlbW90ZUlkKSB7XG4gICAgaWYgKCFjaGFubmVscy5oYXMocmVtb3RlSWQpKSB7XG4gICAgICBMb2dnZXIud2FybmluZyhcbiAgICAgICAgICAnTm8gUGVlckNvbm5lY3Rpb24gYmV0d2VlbiBjdXJyZW50IGVuZHBvaW50IGFuZCBzcGVjaWZpYyByZW1vdGUgJyArXG4gICAgICAgICAgJ2VuZHBvaW50LicsXG4gICAgICApO1xuICAgICAgcmV0dXJuO1xuICAgIH1cbiAgICBjaGFubmVscy5nZXQocmVtb3RlSWQpLnN0b3AoKTtcbiAgICBjaGFubmVscy5kZWxldGUocmVtb3RlSWQpO1xuICB9O1xuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gZ2V0U3RhdHNcbiAgICogQGluc3RhbmNlXG4gICAqIEBkZXNjIEdldCBzdGF0cyBvZiB1bmRlcmx5aW5nIFBlZXJDb25uZWN0aW9uLlxuICAgKiBAbWVtYmVyb2YgT3d0LlAyUC5QMlBDbGllbnRcbiAgICogQHBhcmFtIHtzdHJpbmd9IHJlbW90ZUlkIFJlbW90ZSBlbmRwb2ludCdzIElELlxuICAgKiBAcmV0dXJuIHtQcm9taXNlPFJUQ1N0YXRzUmVwb3J0LCBFcnJvcj59IEl0IHJldHVybnMgYSBwcm9taXNlIHJlc29sdmVkIHdpdGggYW4gUlRDU3RhdHNSZXBvcnQgb3IgcmVqZWN0IHdpdGggYW4gRXJyb3IgaWYgdGhlcmUgaXMgbm8gY29ubmVjdGlvbiB3aXRoIHNwZWNpZmljIHVzZXIuXG4gICAqL1xuICB0aGlzLmdldFN0YXRzID0gZnVuY3Rpb24ocmVtb3RlSWQpIHtcbiAgICBpZiAoIWNoYW5uZWxzLmhhcyhyZW1vdGVJZCkpIHtcbiAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgRXJyb3JNb2R1bGUuUDJQRXJyb3IoXG4gICAgICAgICAgRXJyb3JNb2R1bGUuZXJyb3JzLlAyUF9DTElFTlRfSU5WQUxJRF9TVEFURSxcbiAgICAgICAgICAnTm8gUGVlckNvbm5lY3Rpb24gYmV0d2VlbiBjdXJyZW50IGVuZHBvaW50IGFuZCBzcGVjaWZpYyByZW1vdGUgJyArXG4gICAgICAgICAgJ2VuZHBvaW50LicpKTtcbiAgICB9XG4gICAgcmV0dXJuIGNoYW5uZWxzLmdldChyZW1vdGVJZCkuZ2V0U3RhdHMoKTtcbiAgfTtcblxuICAvKipcbiAgICogQGZ1bmN0aW9uIGdldFBlZXJDb25uZWN0aW9uXG4gICAqIEBpbnN0YW5jZVxuICAgKiBAZGVzYyBHZXQgdW5kZXJseWluZyBQZWVyQ29ubmVjdGlvbi5cbiAgICogQG1lbWJlcm9mIE93dC5QMlAuUDJQQ2xpZW50XG4gICAqIEBwYXJhbSB7c3RyaW5nfSByZW1vdGVJZCBSZW1vdGUgZW5kcG9pbnQncyBJRC5cbiAgICogQHJldHVybiB7UHJvbWlzZTxSVENQZWVyQ29ubmVjdGlvbiwgRXJyb3I+fSBJdCByZXR1cm5zIGEgcHJvbWlzZSByZXNvbHZlZFxuICAgKiAgICAgd2l0aCBhbiBSVENQZWVyQ29ubmVjdGlvbiBvciByZWplY3Qgd2l0aCBhbiBFcnJvciBpZiB0aGVyZSBpcyBub1xuICAgKiAgICAgY29ubmVjdGlvbiB3aXRoIHNwZWNpZmljIHVzZXIuXG4gICAqIEBwcml2YXRlXG4gICAqL1xuICB0aGlzLmdldFBlZXJDb25uZWN0aW9uID0gZnVuY3Rpb24ocmVtb3RlSWQpIHtcbiAgICBpZiAoIWNoYW5uZWxzLmhhcyhyZW1vdGVJZCkpIHtcbiAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgRXJyb3JNb2R1bGUuUDJQRXJyb3IoXG4gICAgICAgICAgRXJyb3JNb2R1bGUuZXJyb3JzLlAyUF9DTElFTlRfSU5WQUxJRF9TVEFURSxcbiAgICAgICAgICAnTm8gUGVlckNvbm5lY3Rpb24gYmV0d2VlbiBjdXJyZW50IGVuZHBvaW50IGFuZCBzcGVjaWZpYyByZW1vdGUgJyArXG4gICAgICAgICAgICAgICdlbmRwb2ludC4nKSk7XG4gICAgfVxuICAgIHJldHVybiBjaGFubmVscy5nZXQocmVtb3RlSWQpLnBlZXJDb25uZWN0aW9uO1xuICB9O1xuXG4gIGNvbnN0IHNlbmRTaWduYWxpbmdNZXNzYWdlID0gZnVuY3Rpb24ocmVtb3RlSWQsIGNvbm5lY3Rpb25JZCwgdHlwZSwgbWVzc2FnZSkge1xuICAgIGNvbnN0IG1zZyA9IHtcbiAgICAgIHR5cGUsXG4gICAgICBjb25uZWN0aW9uSWQsXG4gICAgfTtcbiAgICBpZiAobWVzc2FnZSkge1xuICAgICAgbXNnLmRhdGEgPSBtZXNzYWdlO1xuICAgIH1cbiAgICByZXR1cm4gc2lnbmFsaW5nLnNlbmQocmVtb3RlSWQsIEpTT04uc3RyaW5naWZ5KG1zZykpLmNhdGNoKChlKSA9PiB7XG4gICAgICBpZiAodHlwZW9mIGUgPT09ICdudW1iZXInKSB7XG4gICAgICAgIHRocm93IEVycm9yTW9kdWxlLmdldEVycm9yQnlDb2RlKGUpO1xuICAgICAgfVxuICAgIH0pO1xuICB9O1xuXG4gIC8vIFJldHVybiB0cnVlIGlmIGN1cnJlbnQgZW5kcG9pbnQgaXMgYW4gaW1wb2xpdGUgcGVlciwgd2hpY2ggY29udHJvbHMgdGhlXG4gIC8vIHNlc3Npb24uXG4gIGNvbnN0IGlzUG9saXRlUGVlciA9IGZ1bmN0aW9uKHJlbW90ZUlkKSB7XG4gICAgcmV0dXJuIG15SWQgPCByZW1vdGVJZDtcbiAgfTtcblxuICAvLyBJZiBhIGNvbm5lY3Rpb24gd2l0aCByZW1vdGVJZCB3aXRoIGEgZGlmZmVyZW50IHNlc3Npb24gSUQgZXhpc3RzLCBpdCB3aWxsXG4gIC8vIGJlIHN0b3BwZWQgYW5kIGEgbmV3IGNvbm5lY3Rpb24gd2lsbCBiZSBjcmVhdGVkLlxuICBjb25zdCBnZXRPckNyZWF0ZUNoYW5uZWwgPSBmdW5jdGlvbihyZW1vdGVJZCwgY29ubmVjdGlvbklkKSB7XG4gICAgLy8gSWYgYGNvbm5lY3Rpb25JZGAgaXMgbm90IGRlZmluZWQsIHVzZSB0aGUgbGF0ZXN0IG9uZSBvciBnZW5lcmF0ZSBhIG5ld1xuICAgIC8vIG9uZS5cbiAgICBpZiAoIWNvbm5lY3Rpb25JZCAmJiBjb25uZWN0aW9uSWRzLmhhcyhyZW1vdGVJZCkpIHtcbiAgICAgIGNvbm5lY3Rpb25JZCA9IGNvbm5lY3Rpb25JZHMuZ2V0KHJlbW90ZUlkKTtcbiAgICB9XG4gICAgLy8gRGVsZXRlIG9sZCBjaGFubmVsIGlmIGNvbm5lY3Rpb24gZG9lc24ndCBtYXRjaC5cbiAgICBpZiAoY29ubmVjdGlvbklkcy5oYXMocmVtb3RlSWQpICYmXG4gICAgICAgIGNvbm5lY3Rpb25JZHMuZ2V0KHJlbW90ZUlkKSAhPSBjb25uZWN0aW9uSWQpIHtcbiAgICAgIHNlbGYuc3RvcChyZW1vdGVJZCk7XG4gICAgfVxuICAgIGlmICghY29ubmVjdGlvbklkKSB7XG4gICAgICBjb25zdCBjb25uZWN0aW9uSWRMaW1pdCA9IDEwMDAwMDtcbiAgICAgIGNvbm5lY3Rpb25JZCA9IE1hdGgucm91bmQoTWF0aC5yYW5kb20oKSAqIGNvbm5lY3Rpb25JZExpbWl0KTtcbiAgICB9XG4gICAgY29ubmVjdGlvbklkcy5zZXQocmVtb3RlSWQsIGNvbm5lY3Rpb25JZCk7XG4gICAgaWYgKCFjaGFubmVscy5oYXMocmVtb3RlSWQpKSB7XG4gICAgICAvLyBDb25zdHJ1Y3QgYW4gc2lnbmFsaW5nIHNlbmRlci9yZWNlaXZlciBmb3IgUDJQUGVlckNvbm5lY3Rpb24uXG4gICAgICBjb25zdCBzaWduYWxpbmdGb3JDaGFubmVsID0gT2JqZWN0LmNyZWF0ZShFdmVudERpc3BhdGNoZXIpO1xuICAgICAgc2lnbmFsaW5nRm9yQ2hhbm5lbC5zZW5kU2lnbmFsaW5nTWVzc2FnZSA9IHNlbmRTaWduYWxpbmdNZXNzYWdlO1xuICAgICAgY29uc3QgcGNjID0gbmV3IFAyUFBlZXJDb25uZWN0aW9uQ2hhbm5lbChcbiAgICAgICAgICBjb25maWcsIG15SWQsIHJlbW90ZUlkLCBjb25uZWN0aW9uSWQsIHNpZ25hbGluZ0ZvckNoYW5uZWwpO1xuICAgICAgcGNjLmFkZEV2ZW50TGlzdGVuZXIoJ3N0cmVhbWFkZGVkJywgKHN0cmVhbUV2ZW50KT0+e1xuICAgICAgICBzZWxmLmRpc3BhdGNoRXZlbnQoc3RyZWFtRXZlbnQpO1xuICAgICAgfSk7XG4gICAgICBwY2MuYWRkRXZlbnRMaXN0ZW5lcignbWVzc2FnZXJlY2VpdmVkJywgKG1lc3NhZ2VFdmVudCk9PntcbiAgICAgICAgc2VsZi5kaXNwYXRjaEV2ZW50KG1lc3NhZ2VFdmVudCk7XG4gICAgICB9KTtcbiAgICAgIHBjYy5hZGRFdmVudExpc3RlbmVyKCdlbmRlZCcsICgpID0+IHtcbiAgICAgICAgaWYgKGNoYW5uZWxzLmhhcyhyZW1vdGVJZCkpIHtcbiAgICAgICAgICBjaGFubmVscy5kZWxldGUocmVtb3RlSWQpO1xuICAgICAgICB9XG4gICAgICAgIGNvbm5lY3Rpb25JZHMuZGVsZXRlKHJlbW90ZUlkKTtcbiAgICAgIH0pO1xuICAgICAgY2hhbm5lbHMuc2V0KHJlbW90ZUlkLCBwY2MpO1xuICAgIH1cbiAgICByZXR1cm4gY2hhbm5lbHMuZ2V0KHJlbW90ZUlkKTtcbiAgfTtcbn07XG5cbmV4cG9ydCBkZWZhdWx0IFAyUENsaWVudDtcbiIsIi8vIENvcHlyaWdodCAoQykgPDIwMTg+IEludGVsIENvcnBvcmF0aW9uXG4vL1xuLy8gU1BEWC1MaWNlbnNlLUlkZW50aWZpZXI6IEFwYWNoZS0yLjBcblxuLy8gVGhpcyBmaWxlIGRvZXNuJ3QgaGF2ZSBwdWJsaWMgQVBJcy5cbi8qIGVzbGludC1kaXNhYmxlIHZhbGlkLWpzZG9jICovXG4vKiBlc2xpbnQtZGlzYWJsZSByZXF1aXJlLWpzZG9jICovXG4vKiBnbG9iYWwgRXZlbnQsIE1hcCwgUHJvbWlzZSwgUlRDSWNlQ2FuZGlkYXRlLCBuYXZpZ2F0b3IgKi9cblxuJ3VzZSBzdHJpY3QnO1xuXG5pbXBvcnQgTG9nZ2VyIGZyb20gJy4uL2Jhc2UvbG9nZ2VyLmpzJztcbmltcG9ydCB7RXZlbnREaXNwYXRjaGVyLCBNZXNzYWdlRXZlbnQsIE93dEV2ZW50fSBmcm9tICcuLi9iYXNlL2V2ZW50LmpzJztcbmltcG9ydCB7UHVibGljYXRpb259IGZyb20gJy4uL2Jhc2UvcHVibGljYXRpb24uanMnO1xuaW1wb3J0ICogYXMgVXRpbHMgZnJvbSAnLi4vYmFzZS91dGlscy5qcyc7XG5pbXBvcnQgKiBhcyBFcnJvck1vZHVsZSBmcm9tICcuL2Vycm9yLmpzJztcbmltcG9ydCAqIGFzIFN0cmVhbU1vZHVsZSBmcm9tICcuLi9iYXNlL3N0cmVhbS5qcyc7XG5pbXBvcnQgKiBhcyBTZHBVdGlscyBmcm9tICcuLi9iYXNlL3NkcHV0aWxzLmpzJztcbmltcG9ydCB7VHJhbnNwb3J0U2V0dGluZ3MsIFRyYW5zcG9ydFR5cGV9IGZyb20gJy4uL2Jhc2UvdHJhbnNwb3J0LmpzJztcblxuLyoqXG4gKiBAY2xhc3MgUDJQUGVlckNvbm5lY3Rpb25DaGFubmVsRXZlbnRcbiAqIEBkZXNjIEV2ZW50IGZvciBTdHJlYW0uXG4gKiBAbWVtYmVyT2YgT3d0LlAyUFxuICogQHByaXZhdGVcbiAqICovXG5leHBvcnQgY2xhc3MgUDJQUGVlckNvbm5lY3Rpb25DaGFubmVsRXZlbnQgZXh0ZW5kcyBFdmVudCB7XG4gIC8qIGVzbGludC1kaXNhYmxlLW5leHQtbGluZSByZXF1aXJlLWpzZG9jICovXG4gIGNvbnN0cnVjdG9yKGluaXQpIHtcbiAgICBzdXBlcihpbml0KTtcbiAgICB0aGlzLnN0cmVhbSA9IGluaXQuc3RyZWFtO1xuICB9XG59XG5cbmNvbnN0IERhdGFDaGFubmVsTGFiZWwgPSB7XG4gIE1FU1NBR0U6ICdtZXNzYWdlJyxcbiAgRklMRTogJ2ZpbGUnLFxufTtcblxuY29uc3QgU2lnbmFsaW5nVHlwZSA9IHtcbiAgREVOSUVEOiAnY2hhdC1kZW5pZWQnLFxuICBDTE9TRUQ6ICdjaGF0LWNsb3NlZCcsXG4gIE5FR09USUFUSU9OX05FRURFRDogJ2NoYXQtbmVnb3RpYXRpb24tbmVlZGVkJyxcbiAgVFJBQ0tfU09VUkNFUzogJ2NoYXQtdHJhY2stc291cmNlcycsXG4gIFNUUkVBTV9JTkZPOiAnY2hhdC1zdHJlYW0taW5mbycsXG4gIFNEUDogJ2NoYXQtc2lnbmFsJyxcbiAgVFJBQ0tTX0FEREVEOiAnY2hhdC10cmFja3MtYWRkZWQnLFxuICBUUkFDS1NfUkVNT1ZFRDogJ2NoYXQtdHJhY2tzLXJlbW92ZWQnLFxuICBEQVRBX1JFQ0VJVkVEOiAnY2hhdC1kYXRhLXJlY2VpdmVkJyxcbiAgVUE6ICdjaGF0LXVhJyxcbn07XG5cbmNvbnN0IHN5c0luZm8gPSBVdGlscy5zeXNJbmZvKCk7XG5cbi8qKlxuICogQGNsYXNzIFAyUFBlZXJDb25uZWN0aW9uQ2hhbm5lbFxuICogQGRlc2MgQSBQMlBQZWVyQ29ubmVjdGlvbkNoYW5uZWwgbWFuYWdlcyBhIFBlZXJDb25uZWN0aW9uIG9iamVjdCwgaGFuZGxlcyBhbGxcbiAqIGludGVyYWN0aW9ucyBiZXR3ZWVuIHRoaXMgZW5kcG9pbnQgKGxvY2FsKSBhbmQgYSByZW1vdGUgZW5kcG9pbnQuIE9ubHkgb25lXG4gKiBQZWVyQ29ubmVjdGlvbkNoYW5uZWwgaXMgYWxpdmUgZm9yIGEgbG9jYWwgLSByZW1vdGUgZW5kcG9pbnQgcGFpciBhdCBhbnlcbiAqIGdpdmVuIHRpbWUuXG4gKiBAbWVtYmVyT2YgT3d0LlAyUFxuICogQHByaXZhdGVcbiAqL1xuY2xhc3MgUDJQUGVlckNvbm5lY3Rpb25DaGFubmVsIGV4dGVuZHMgRXZlbnREaXNwYXRjaGVyIHtcbiAgLy8gfHNpZ25hbGluZ3wgaXMgYW4gb2JqZWN0IGhhcyBhIG1ldGhvZCB8c2VuZFNpZ25hbGluZ01lc3NhZ2V8LlxuICAvKiBlc2xpbnQtZGlzYWJsZS1uZXh0LWxpbmUgcmVxdWlyZS1qc2RvYyAqL1xuICBjb25zdHJ1Y3RvcihcbiAgICAgIGNvbmZpZywgbG9jYWxJZCwgcmVtb3RlSWQsIGNvbm5lY3Rpb25JZCwgc2lnbmFsaW5nKSB7XG4gICAgc3VwZXIoKTtcbiAgICB0aGlzLl9jb25maWcgPSBjb25maWc7XG4gICAgdGhpcy5fcmVtb3RlSWQgPSByZW1vdGVJZDtcbiAgICB0aGlzLl9jb25uZWN0aW9uSWQgPSBjb25uZWN0aW9uSWQ7XG4gICAgdGhpcy5fc2lnbmFsaW5nID0gc2lnbmFsaW5nO1xuICAgIHRoaXMuX3BjID0gbnVsbDtcbiAgICB0aGlzLl9wdWJsaXNoZWRTdHJlYW1zID0gbmV3IE1hcCgpOyAvLyBLZXkgaXMgc3RyZWFtcyBwdWJsaXNoZWQsIHZhbHVlIGlzIGl0cyBwdWJsaWNhdGlvbi5cbiAgICB0aGlzLl9wZW5kaW5nU3RyZWFtcyA9IFtdOyAvLyBTdHJlYW1zIGdvaW5nIHRvIGJlIGFkZGVkIHRvIFBlZXJDb25uZWN0aW9uLlxuICAgIHRoaXMuX3B1Ymxpc2hpbmdTdHJlYW1zID0gW107IC8vIFN0cmVhbXMgaGF2ZSBiZWVuIGFkZGVkIHRvIFBlZXJDb25uZWN0aW9uLCBidXQgZG9lcyBub3QgcmVjZWl2ZSBhY2sgZnJvbSByZW1vdGUgc2lkZS5cbiAgICB0aGlzLl9wZW5kaW5nVW5wdWJsaXNoU3RyZWFtcyA9IFtdOyAvLyBTdHJlYW1zIGdvaW5nIHRvIGJlIHJlbW92ZWQuXG4gICAgLy8gS2V5IGlzIE1lZGlhU3RyZWFtJ3MgSUQsIHZhbHVlIGlzIGFuIG9iamVjdCB7c291cmNlOnthdWRpbzpzdHJpbmcsIHZpZGVvOnN0cmluZ30sIGF0dHJpYnV0ZXM6IG9iamVjdCwgc3RyZWFtOiBSZW1vdGVTdHJlYW0sIG1lZGlhU3RyZWFtOiBNZWRpYVN0cmVhbX0uIGBzdHJlYW1gIGFuZCBgbWVkaWFTdHJlYW1gIHdpbGwgYmUgc2V0IHdoZW4gYHRyYWNrYCBldmVudCBpcyBmaXJlZCBvbiBgUlRDUGVlckNvbm5lY3Rpb25gLiBgbWVkaWFTdHJlYW1gIHdpbGwgYmUgYG51bGxgIGFmdGVyIGBzdHJlYW1hZGRlZGAgZXZlbnQgaXMgZmlyZWQgb24gYFAyUENsaWVudGAuIE90aGVyIHByb3BlcnRlcyB3aWxsIGJlIHNldCB1cG9uIGBTVFJFQU1fSU5GT2AgZXZlbnQgZnJvbSBzaWduYWxpbmcgY2hhbm5lbC5cbiAgICB0aGlzLl9yZW1vdGVTdHJlYW1JbmZvID0gbmV3IE1hcCgpO1xuICAgIHRoaXMuX3JlbW90ZVN0cmVhbXMgPSBbXTtcbiAgICB0aGlzLl9yZW1vdGVUcmFja1NvdXJjZUluZm8gPSBuZXcgTWFwKCk7IC8vIEtleSBpcyBNZWRpYVN0cmVhbVRyYWNrJ3MgSUQsIHZhbHVlIGlzIHNvdXJjZSBpbmZvLlxuICAgIHRoaXMuX3B1Ymxpc2hQcm9taXNlcyA9IG5ldyBNYXAoKTsgLy8gS2V5IGlzIE1lZGlhU3RyZWFtJ3MgSUQsIHZhbHVlIGlzIGFuIG9iamVjdCBoYXMgfHJlc29sdmV8IGFuZCB8cmVqZWN0fC5cbiAgICB0aGlzLl91bnB1Ymxpc2hQcm9taXNlcyA9IG5ldyBNYXAoKTsgLy8gS2V5IGlzIE1lZGlhU3RyZWFtJ3MgSUQsIHZhbHVlIGlzIGFuIG9iamVjdCBoYXMgfHJlc29sdmV8IGFuZCB8cmVqZWN0fC5cbiAgICB0aGlzLl9wdWJsaXNoaW5nU3RyZWFtVHJhY2tzID0gbmV3IE1hcCgpOyAvLyBLZXkgaXMgTWVkaWFTdHJlYW0ncyBJRCwgdmFsdWUgaXMgYW4gYXJyYXkgb2YgdGhlIElEIG9mIGl0cyBNZWRpYVN0cmVhbVRyYWNrcyB0aGF0IGhhdmVuJ3QgYmVlbiBhY2tlZC5cbiAgICB0aGlzLl9wdWJsaXNoZWRTdHJlYW1UcmFja3MgPSBuZXcgTWFwKCk7IC8vIEtleSBpcyBNZWRpYVN0cmVhbSdzIElELCB2YWx1ZSBpcyBhbiBhcnJheSBvZiB0aGUgSUQgb2YgaXRzIE1lZGlhU3RyZWFtVHJhY2tzIHRoYXQgaGF2ZW4ndCBiZWVuIHJlbW92ZWQuXG4gICAgdGhpcy5faXNOZWdvdGlhdGlvbk5lZWRlZCA9IGZhbHNlO1xuICAgIHRoaXMuX3JlbW90ZVNpZGVTdXBwb3J0c1JlbW92ZVN0cmVhbSA9IHRydWU7XG4gICAgdGhpcy5fcmVtb3RlU2lkZUlnbm9yZXNEYXRhQ2hhbm5lbEFja3MgPSBmYWxzZTtcbiAgICB0aGlzLl9yZW1vdGVJY2VDYW5kaWRhdGVzID0gW107XG4gICAgdGhpcy5fZGF0YUNoYW5uZWxzID0gbmV3IE1hcCgpOyAvLyBLZXkgaXMgZGF0YSBjaGFubmVsJ3MgbGFiZWwsIHZhbHVlIGlzIGEgUlRDRGF0YUNoYW5uZWwuXG4gICAgdGhpcy5fcGVuZGluZ01lc3NhZ2VzID0gW107XG4gICAgdGhpcy5fZGF0YVNlcSA9IDE7IC8vIFNlcXVlbmNlIG51bWJlciBmb3IgZGF0YSBjaGFubmVsIG1lc3NhZ2VzLlxuICAgIHRoaXMuX3NlbmREYXRhUHJvbWlzZXMgPSBuZXcgTWFwKCk7IC8vIEtleSBpcyBkYXRhIHNlcXVlbmNlIG51bWJlciwgdmFsdWUgaXMgYW4gb2JqZWN0IGhhcyB8cmVzb2x2ZXwgYW5kIHxyZWplY3R8LlxuICAgIHRoaXMuX2FkZGVkVHJhY2tJZHMgPSBbXTsgLy8gVHJhY2tzIHRoYXQgaGF2ZSBiZWVuIGFkZGVkIGFmdGVyIHJlY2VpdmluZyByZW1vdGUgU0RQIGJ1dCBiZWZvcmUgY29ubmVjdGlvbiBpcyBlc3RhYmxpc2hlZC4gRHJhaW5pbmcgdGhlc2UgbWVzc2FnZXMgd2hlbiBJQ0UgY29ubmVjdGlvbiBzdGF0ZSBpcyBjb25uZWN0ZWQuXG4gICAgdGhpcy5faXNQb2xpdGVQZWVyID0gbG9jYWxJZCA8IHJlbW90ZUlkO1xuICAgIHRoaXMuX3NldHRpbmdMb2NhbFNkcCA9IGZhbHNlO1xuICAgIHRoaXMuX3NldHRpbmdSZW1vdGVTZHAgPSBmYWxzZTtcbiAgICB0aGlzLl9kaXNwb3NlZCA9IGZhbHNlO1xuICAgIHRoaXMuX2NyZWF0ZVBlZXJDb25uZWN0aW9uKCk7XG4gICAgdGhpcy5fc2VuZFVhKHN5c0luZm8pO1xuICB9XG5cbiAgZ2V0IHBlZXJDb25uZWN0aW9uKCkge1xuICAgIHJldHVybiB0aGlzLl9wYztcbiAgfVxuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gcHVibGlzaFxuICAgKiBAZGVzYyBQdWJsaXNoIGEgc3RyZWFtIHRvIHRoZSByZW1vdGUgZW5kcG9pbnQuXG4gICAqIEBwcml2YXRlXG4gICAqL1xuICBwdWJsaXNoKHN0cmVhbSkge1xuICAgIGlmICghKHN0cmVhbSBpbnN0YW5jZW9mIFN0cmVhbU1vZHVsZS5Mb2NhbFN0cmVhbSkpIHtcbiAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgVHlwZUVycm9yKCdJbnZhbGlkIHN0cmVhbS4nKSk7XG4gICAgfVxuICAgIGlmICh0aGlzLl9wdWJsaXNoZWRTdHJlYW1zLmhhcyhzdHJlYW0pKSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IEVycm9yTW9kdWxlLlAyUEVycm9yKFxuICAgICAgICAgIEVycm9yTW9kdWxlLmVycm9ycy5QMlBfQ0xJRU5UX0lMTEVHQUxfQVJHVU1FTlQsXG4gICAgICAgICAgJ0R1cGxpY2F0ZWQgc3RyZWFtLicpKTtcbiAgICB9XG4gICAgaWYgKHRoaXMuX2FyZUFsbFRyYWNrc0VuZGVkKHN0cmVhbS5tZWRpYVN0cmVhbSkpIHtcbiAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgRXJyb3JNb2R1bGUuUDJQRXJyb3IoXG4gICAgICAgICAgRXJyb3JNb2R1bGUuZXJyb3JzLlAyUF9DTElFTlRfSU5WQUxJRF9TVEFURSxcbiAgICAgICAgICAnQWxsIHRyYWNrcyBhcmUgZW5kZWQuJykpO1xuICAgIH1cbiAgICByZXR1cm4gdGhpcy5fc2VuZFN0cmVhbUluZm8oc3RyZWFtKS50aGVuKCgpID0+IHtcbiAgICAgIHJldHVybiBuZXcgUHJvbWlzZSgocmVzb2x2ZSwgcmVqZWN0KSA9PiB7XG4gICAgICAgIHRoaXMuX2FkZFN0cmVhbShzdHJlYW0ubWVkaWFTdHJlYW0pO1xuICAgICAgICB0aGlzLl9wdWJsaXNoaW5nU3RyZWFtcy5wdXNoKHN0cmVhbSk7XG4gICAgICAgIGNvbnN0IHRyYWNrSWRzID0gQXJyYXkuZnJvbShzdHJlYW0ubWVkaWFTdHJlYW0uZ2V0VHJhY2tzKCksXG4gICAgICAgICAgICAodHJhY2spID0+IHRyYWNrLmlkKTtcbiAgICAgICAgdGhpcy5fcHVibGlzaGluZ1N0cmVhbVRyYWNrcy5zZXQoc3RyZWFtLm1lZGlhU3RyZWFtLmlkLFxuICAgICAgICAgICAgdHJhY2tJZHMpO1xuICAgICAgICB0aGlzLl9wdWJsaXNoUHJvbWlzZXMuc2V0KHN0cmVhbS5tZWRpYVN0cmVhbS5pZCwge1xuICAgICAgICAgIHJlc29sdmU6IHJlc29sdmUsXG4gICAgICAgICAgcmVqZWN0OiByZWplY3QsXG4gICAgICAgIH0pO1xuICAgICAgfSk7XG4gICAgfSk7XG4gIH1cblxuICAvKipcbiAgICogQGZ1bmN0aW9uIHNlbmRcbiAgICogQGRlc2MgU2VuZCBhIG1lc3NhZ2UgdG8gdGhlIHJlbW90ZSBlbmRwb2ludC5cbiAgICogQHByaXZhdGVcbiAgICovXG4gIHNlbmQobWVzc2FnZSkge1xuICAgIGlmICghKHR5cGVvZiBtZXNzYWdlID09PSAnc3RyaW5nJykpIHtcbiAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgVHlwZUVycm9yKCdJbnZhbGlkIG1lc3NhZ2UuJykpO1xuICAgIH1cbiAgICBjb25zdCBkYXRhID0ge1xuICAgICAgaWQ6IHRoaXMuX2RhdGFTZXErKyxcbiAgICAgIGRhdGE6IG1lc3NhZ2UsXG4gICAgfTtcbiAgICBpZiAoIXRoaXMuX2RhdGFDaGFubmVscy5oYXMoRGF0YUNoYW5uZWxMYWJlbC5NRVNTQUdFKSkge1xuICAgICAgdGhpcy5fY3JlYXRlRGF0YUNoYW5uZWwoRGF0YUNoYW5uZWxMYWJlbC5NRVNTQUdFKTtcbiAgICB9XG5cbiAgICBjb25zdCBkYyA9IHRoaXMuX2RhdGFDaGFubmVscy5nZXQoRGF0YUNoYW5uZWxMYWJlbC5NRVNTQUdFKTtcbiAgICBpZiAoZGMucmVhZHlTdGF0ZSA9PT0gJ29wZW4nKSB7XG4gICAgICB0aGlzLl9kYXRhQ2hhbm5lbHMuZ2V0KERhdGFDaGFubmVsTGFiZWwuTUVTU0FHRSlcbiAgICAgICAgICAuc2VuZChKU09OLnN0cmluZ2lmeShkYXRhKSk7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZXNvbHZlKCk7XG4gICAgfSBlbHNlIHtcbiAgICAgIHRoaXMuX3BlbmRpbmdNZXNzYWdlcy5wdXNoKGRhdGEpO1xuICAgICAgY29uc3QgcHJvbWlzZSA9IG5ldyBQcm9taXNlKChyZXNvbHZlLCByZWplY3QpID0+IHtcbiAgICAgICAgdGhpcy5fc2VuZERhdGFQcm9taXNlcy5zZXQoZGF0YS5pZCwge1xuICAgICAgICAgIHJlc29sdmU6IHJlc29sdmUsXG4gICAgICAgICAgcmVqZWN0OiByZWplY3QsXG4gICAgICAgIH0pO1xuICAgICAgfSk7XG4gICAgICByZXR1cm4gcHJvbWlzZTtcbiAgICB9XG4gIH1cblxuICAvKipcbiAgICogQGZ1bmN0aW9uIHN0b3BcbiAgICogQGRlc2MgU3RvcCB0aGUgY29ubmVjdGlvbiB3aXRoIHJlbW90ZSBlbmRwb2ludC5cbiAgICogQHByaXZhdGVcbiAgICovXG4gIHN0b3AoKSB7XG4gICAgdGhpcy5fc3RvcCh1bmRlZmluZWQsIHRydWUpO1xuICB9XG5cbiAgLyoqXG4gICAqIEBmdW5jdGlvbiBnZXRTdGF0c1xuICAgKiBAZGVzYyBHZXQgc3RhdHMgZm9yIGEgc3BlY2lmaWMgTWVkaWFTdHJlYW0uXG4gICAqIEBwcml2YXRlXG4gICAqL1xuICBnZXRTdGF0cyhtZWRpYVN0cmVhbSkge1xuICAgIGlmICh0aGlzLl9wYykge1xuICAgICAgaWYgKG1lZGlhU3RyZWFtID09PSB1bmRlZmluZWQpIHtcbiAgICAgICAgcmV0dXJuIHRoaXMuX3BjLmdldFN0YXRzKCk7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICBjb25zdCB0cmFja3NTdGF0c1JlcG9ydHMgPSBbXTtcbiAgICAgICAgcmV0dXJuIFByb21pc2UuYWxsKFttZWRpYVN0cmVhbS5nZXRUcmFja3MoKS5mb3JFYWNoKCh0cmFjaykgPT4ge1xuICAgICAgICAgIHRoaXMuX2dldFN0YXRzKHRyYWNrLCB0cmFja3NTdGF0c1JlcG9ydHMpO1xuICAgICAgICB9KV0pLnRoZW4oXG4gICAgICAgICAgICAoKSA9PiB7XG4gICAgICAgICAgICAgIHJldHVybiBuZXcgUHJvbWlzZSgocmVzb2x2ZSwgcmVqZWN0KSA9PiB7XG4gICAgICAgICAgICAgICAgcmVzb2x2ZSh0cmFja3NTdGF0c1JlcG9ydHMpO1xuICAgICAgICAgICAgICB9KTtcbiAgICAgICAgICAgIH0pO1xuICAgICAgfVxuICAgIH0gZWxzZSB7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IEVycm9yTW9kdWxlLlAyUEVycm9yKFxuICAgICAgICAgIEVycm9yTW9kdWxlLmVycm9ycy5QMlBfQ0xJRU5UX0lOVkFMSURfU1RBVEUpKTtcbiAgICB9XG4gIH1cblxuICBfZ2V0U3RhdHMobWVkaWFTdHJlYW1UcmFjaywgcmVwb3J0c1Jlc3VsdCkge1xuICAgIHJldHVybiB0aGlzLl9wYy5nZXRTdGF0cyhtZWRpYVN0cmVhbVRyYWNrKS50aGVuKFxuICAgICAgICAoc3RhdHNSZXBvcnQpID0+IHtcbiAgICAgICAgICByZXBvcnRzUmVzdWx0LnB1c2goc3RhdHNSZXBvcnQpO1xuICAgICAgICB9KTtcbiAgfVxuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gX2FkZFN0cmVhbVxuICAgKiBAZGVzYyBDcmVhdGUgUlRDUnRwU2VuZGVycyBmb3IgYWxsIHRyYWNrcyBpbiB0aGUgc3RyZWFtLlxuICAgKiBAcHJpdmF0ZVxuICAgKi9cbiAgX2FkZFN0cmVhbShzdHJlYW0pIHtcbiAgICBmb3IgKGNvbnN0IHRyYWNrIG9mIHN0cmVhbS5nZXRUcmFja3MoKSkge1xuICAgICAgdGhpcy5fcGMuYWRkVHJhbnNjZWl2ZXIoXG4gICAgICAgICAgdHJhY2ssIHtkaXJlY3Rpb246ICdzZW5kb25seScsIHN0cmVhbXM6IFtzdHJlYW1dfSk7XG4gICAgfVxuICB9XG5cbiAgLyoqXG4gICAqIEBmdW5jdGlvbiBvbk1lc3NhZ2VcbiAgICogQGRlc2MgVGhpcyBtZXRob2QgaXMgY2FsbGVkIGJ5IFAyUENsaWVudCB3aGVuIHRoZXJlIGlzIG5ldyBzaWduYWxpbmcgbWVzc2FnZSBhcnJpdmVkLlxuICAgKiBAcHJpdmF0ZVxuICAgKi9cbiAgb25NZXNzYWdlKG1lc3NhZ2UpIHtcbiAgICB0aGlzLl9TaWduYWxpbmdNZXNzc2FnZUhhbmRsZXIobWVzc2FnZSk7XG4gIH1cblxuICBfc2VuZFNkcChzZHApIHtcbiAgICByZXR1cm4gdGhpcy5fc2lnbmFsaW5nLnNlbmRTaWduYWxpbmdNZXNzYWdlKFxuICAgICAgICB0aGlzLl9yZW1vdGVJZCwgdGhpcy5fY29ubmVjdGlvbklkLCBTaWduYWxpbmdUeXBlLlNEUCwgc2RwKTtcbiAgfVxuXG4gIF9zZW5kVWEoc3lzSW5mbykge1xuICAgIGNvbnN0IHVhID0ge3Nkazogc3lzSW5mby5zZGssIGNhcGFiaWxpdGllczogc3lzSW5mby5jYXBhYmlsaXRpZXN9O1xuICAgIHRoaXMuX3NlbmRTaWduYWxpbmdNZXNzYWdlKFNpZ25hbGluZ1R5cGUuVUEsIHVhKTtcbiAgfVxuXG4gIF9zZW5kU2lnbmFsaW5nTWVzc2FnZSh0eXBlLCBtZXNzYWdlKSB7XG4gICAgcmV0dXJuIHRoaXMuX3NpZ25hbGluZy5zZW5kU2lnbmFsaW5nTWVzc2FnZShcbiAgICAgICAgdGhpcy5fcmVtb3RlSWQsIHRoaXMuX2Nvbm5lY3Rpb25JZCwgdHlwZSwgbWVzc2FnZSk7XG4gIH1cblxuICBfU2lnbmFsaW5nTWVzc3NhZ2VIYW5kbGVyKG1lc3NhZ2UpIHtcbiAgICBMb2dnZXIuZGVidWcoJ0NoYW5uZWwgcmVjZWl2ZWQgbWVzc2FnZTogJyArIG1lc3NhZ2UpO1xuICAgIHN3aXRjaCAobWVzc2FnZS50eXBlKSB7XG4gICAgICBjYXNlIFNpZ25hbGluZ1R5cGUuVUE6XG4gICAgICAgIHRoaXMuX2hhbmRsZVJlbW90ZUNhcGFiaWxpdHkobWVzc2FnZS5kYXRhKTtcbiAgICAgICAgYnJlYWs7XG4gICAgICBjYXNlIFNpZ25hbGluZ1R5cGUuVFJBQ0tfU09VUkNFUzpcbiAgICAgICAgdGhpcy5fdHJhY2tTb3VyY2VzSGFuZGxlcihtZXNzYWdlLmRhdGEpO1xuICAgICAgICBicmVhaztcbiAgICAgIGNhc2UgU2lnbmFsaW5nVHlwZS5TVFJFQU1fSU5GTzpcbiAgICAgICAgdGhpcy5fc3RyZWFtSW5mb0hhbmRsZXIobWVzc2FnZS5kYXRhKTtcbiAgICAgICAgYnJlYWs7XG4gICAgICBjYXNlIFNpZ25hbGluZ1R5cGUuU0RQOlxuICAgICAgICB0aGlzLl9zZHBIYW5kbGVyKG1lc3NhZ2UuZGF0YSk7XG4gICAgICAgIGJyZWFrO1xuICAgICAgY2FzZSBTaWduYWxpbmdUeXBlLlRSQUNLU19BRERFRDpcbiAgICAgICAgdGhpcy5fdHJhY2tzQWRkZWRIYW5kbGVyKG1lc3NhZ2UuZGF0YSk7XG4gICAgICAgIGJyZWFrO1xuICAgICAgY2FzZSBTaWduYWxpbmdUeXBlLlRSQUNLU19SRU1PVkVEOlxuICAgICAgICB0aGlzLl90cmFja3NSZW1vdmVkSGFuZGxlcihtZXNzYWdlLmRhdGEpO1xuICAgICAgICBicmVhaztcbiAgICAgIGNhc2UgU2lnbmFsaW5nVHlwZS5EQVRBX1JFQ0VJVkVEOlxuICAgICAgICB0aGlzLl9kYXRhUmVjZWl2ZWRIYW5kbGVyKG1lc3NhZ2UuZGF0YSk7XG4gICAgICAgIGJyZWFrO1xuICAgICAgY2FzZSBTaWduYWxpbmdUeXBlLkNMT1NFRDpcbiAgICAgICAgdGhpcy5fY2hhdENsb3NlZEhhbmRsZXIobWVzc2FnZS5kYXRhKTtcbiAgICAgICAgYnJlYWs7XG4gICAgICBkZWZhdWx0OlxuICAgICAgICBMb2dnZXIuZXJyb3IoJ0ludmFsaWQgc2lnbmFsaW5nIG1lc3NhZ2UgcmVjZWl2ZWQuIFR5cGU6ICcgK1xuICAgICAgICAgICAgbWVzc2FnZS50eXBlKTtcbiAgICB9XG4gIH1cblxuICAvKipcbiAgICogQGZ1bmN0aW9uIF90cmFja3NBZGRlZEhhbmRsZXJcbiAgICogQGRlc2MgSGFuZGxlIHRyYWNrIGFkZGVkIGV2ZW50IGZyb20gcmVtb3RlIHNpZGUuXG4gICAqIEBwcml2YXRlXG4gICAqL1xuICBfdHJhY2tzQWRkZWRIYW5kbGVyKGlkcykge1xuICAgIC8vIEN1cnJlbnRseSwgfGlkc3wgY29udGFpbnMgYWxsIHRyYWNrIElEcyBvZiBhIE1lZGlhU3RyZWFtLiBGb2xsb3dpbmcgYWxnb3JpdGhtIGFsc28gaGFuZGxlcyB8aWRzfCBpcyBhIHBhcnQgb2YgYSBNZWRpYVN0cmVhbSdzIHRyYWNrcy5cbiAgICBmb3IgKGNvbnN0IGlkIG9mIGlkcykge1xuICAgICAgLy8gSXQgY291bGQgYmUgYSBwcm9ibGVtIGlmIHRoZXJlIGlzIGEgdHJhY2sgcHVibGlzaGVkIHdpdGggZGlmZmVyZW50IE1lZGlhU3RyZWFtcywgbW92aW5nIHRvIG1pZC5cbiAgICAgIHRoaXMuX3B1Ymxpc2hpbmdTdHJlYW1UcmFja3MuZm9yRWFjaCgobWVkaWFUcmFja0lkcywgbWVkaWFTdHJlYW1JZCkgPT4ge1xuICAgICAgICBmb3IgKGxldCBpID0gMDsgaSA8IG1lZGlhVHJhY2tJZHMubGVuZ3RoOyBpKyspIHtcbiAgICAgICAgICBpZiAobWVkaWFUcmFja0lkc1tpXSA9PT0gaWQpIHtcbiAgICAgICAgICAgIC8vIE1vdmUgdGhpcyB0cmFjayBmcm9tIHB1Ymxpc2hpbmcgdHJhY2tzIHRvIHB1Ymxpc2hlZCB0cmFja3MuXG4gICAgICAgICAgICBpZiAoIXRoaXMuX3B1Ymxpc2hlZFN0cmVhbVRyYWNrcy5oYXMobWVkaWFTdHJlYW1JZCkpIHtcbiAgICAgICAgICAgICAgdGhpcy5fcHVibGlzaGVkU3RyZWFtVHJhY2tzLnNldChtZWRpYVN0cmVhbUlkLCBbXSk7XG4gICAgICAgICAgICB9XG4gICAgICAgICAgICB0aGlzLl9wdWJsaXNoZWRTdHJlYW1UcmFja3MuZ2V0KG1lZGlhU3RyZWFtSWQpLnB1c2goXG4gICAgICAgICAgICAgICAgbWVkaWFUcmFja0lkc1tpXSk7XG4gICAgICAgICAgICBtZWRpYVRyYWNrSWRzLnNwbGljZShpLCAxKTtcbiAgICAgICAgICB9XG4gICAgICAgICAgLy8gUmVzb2x2aW5nIGNlcnRhaW4gcHVibGlzaCBwcm9taXNlIHdoZW4gcmVtb3RlIGVuZHBvaW50IHJlY2VpdmVkIGFsbCB0cmFja3Mgb2YgYSBNZWRpYVN0cmVhbS5cbiAgICAgICAgICBpZiAobWVkaWFUcmFja0lkcy5sZW5ndGggPT0gMCkge1xuICAgICAgICAgICAgaWYgKCF0aGlzLl9wdWJsaXNoUHJvbWlzZXMuaGFzKG1lZGlhU3RyZWFtSWQpKSB7XG4gICAgICAgICAgICAgIExvZ2dlci53YXJuaW5nKCdDYW5ub3QgZmluZCB0aGUgcHJvbWlzZSBmb3IgcHVibGlzaGluZyAnICtcbiAgICAgICAgICAgICAgICBtZWRpYVN0cmVhbUlkKTtcbiAgICAgICAgICAgICAgY29udGludWU7XG4gICAgICAgICAgICB9XG4gICAgICAgICAgICBjb25zdCB0YXJnZXRTdHJlYW1JbmRleCA9IHRoaXMuX3B1Ymxpc2hpbmdTdHJlYW1zLmZpbmRJbmRleChcbiAgICAgICAgICAgICAgICAoZWxlbWVudCkgPT4gZWxlbWVudC5tZWRpYVN0cmVhbS5pZCA9PSBtZWRpYVN0cmVhbUlkKTtcbiAgICAgICAgICAgIGNvbnN0IHRhcmdldFN0cmVhbSA9IHRoaXMuX3B1Ymxpc2hpbmdTdHJlYW1zW3RhcmdldFN0cmVhbUluZGV4XTtcbiAgICAgICAgICAgIHRoaXMuX3B1Ymxpc2hpbmdTdHJlYW1zLnNwbGljZSh0YXJnZXRTdHJlYW1JbmRleCwgMSk7XG5cbiAgICAgICAgICAgIC8vIFNldCB0cmFuc2NlaXZlcnMgZm9yIFB1YmxpY2F0aW9uLlxuICAgICAgICAgICAgY29uc3QgdHJhbnNwb3J0ID1cbiAgICAgICAgICAgICAgICBuZXcgVHJhbnNwb3J0U2V0dGluZ3MoVHJhbnNwb3J0VHlwZS5XRUJSVEMsIHVuZGVmaW5lZCk7XG4gICAgICAgICAgICB0cmFuc3BvcnQucnRwVHJhbnNjZWl2ZXJzID0gW107XG4gICAgICAgICAgICBmb3IgKGNvbnN0IHRyYW5zY2VpdmVyIG9mIHRoaXMuX3BjLmdldFRyYW5zY2VpdmVycygpKSB7XG4gICAgICAgICAgICAgIGlmICh0cmFuc2NlaXZlci5zZW5kZXI/LnRyYWNrIGluXG4gICAgICAgICAgICAgICAgICB0aGlzLl9wdWJsaXNoZWRTdHJlYW1UcmFja3MuZ2V0KG1lZGlhU3RyZWFtSWQpKSB7XG4gICAgICAgICAgICAgICAgdHJhbnNwb3J0LnJ0cFRyYW5zY2VpdmVycy5wdXNoKHRyYW5zY2VpdmVyKTtcbiAgICAgICAgICAgICAgfVxuICAgICAgICAgICAgfVxuXG4gICAgICAgICAgICBjb25zdCBwdWJsaWNhdGlvbiA9IG5ldyBQdWJsaWNhdGlvbihcbiAgICAgICAgICAgICAgICBpZCwgdHJhbnNwb3J0LCAoKSA9PiB7XG4gICAgICAgICAgICAgICAgICB0aGlzLl91bnB1Ymxpc2godGFyZ2V0U3RyZWFtKS50aGVuKCgpID0+IHtcbiAgICAgICAgICAgICAgICAgICAgcHVibGljYXRpb24uZGlzcGF0Y2hFdmVudChuZXcgT3d0RXZlbnQoJ2VuZGVkJykpO1xuICAgICAgICAgICAgICAgICAgfSwgKGVycikgPT4ge1xuICAgICAgICAgICAgICAgICAgLy8gVXNlIGRlYnVnIG1vZGUgYmVjYXVzZSB0aGlzIGVycm9yIHVzdWFsbHkgZG9lc24ndCBibG9jayBzdG9wcGluZyBhIHB1YmxpY2F0aW9uLlxuICAgICAgICAgICAgICAgICAgICBMb2dnZXIuZGVidWcoXG4gICAgICAgICAgICAgICAgICAgICAgICAnU29tZXRoaW5nIHdyb25nIGhhcHBlbmVkIGR1cmluZyBzdG9wcGluZyBhICcrXG4gICAgICAgICAgICAgICAgICAgICAgICAncHVibGljYXRpb24uICcgKyBlcnIubWVzc2FnZSk7XG4gICAgICAgICAgICAgICAgICB9KTtcbiAgICAgICAgICAgICAgICB9LCAoKSA9PiB7XG4gICAgICAgICAgICAgICAgICBpZiAoIXRhcmdldFN0cmVhbSB8fCAhdGFyZ2V0U3RyZWFtLm1lZGlhU3RyZWFtKSB7XG4gICAgICAgICAgICAgICAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgRXJyb3JNb2R1bGUuUDJQRXJyb3IoXG4gICAgICAgICAgICAgICAgICAgICAgICBFcnJvck1vZHVsZS5lcnJvcnMuUDJQX0NMSUVOVF9JTlZBTElEX1NUQVRFLFxuICAgICAgICAgICAgICAgICAgICAgICAgJ1B1YmxpY2F0aW9uIGlzIG5vdCBhdmFpbGFibGUuJykpO1xuICAgICAgICAgICAgICAgICAgfVxuICAgICAgICAgICAgICAgICAgcmV0dXJuIHRoaXMuZ2V0U3RhdHModGFyZ2V0U3RyZWFtLm1lZGlhU3RyZWFtKTtcbiAgICAgICAgICAgICAgICB9KTtcbiAgICAgICAgICAgIHRoaXMuX3B1Ymxpc2hlZFN0cmVhbXMuc2V0KHRhcmdldFN0cmVhbSwgcHVibGljYXRpb24pO1xuICAgICAgICAgICAgdGhpcy5fcHVibGlzaFByb21pc2VzLmdldChtZWRpYVN0cmVhbUlkKS5yZXNvbHZlKHB1YmxpY2F0aW9uKTtcbiAgICAgICAgICAgIHRoaXMuX3B1Ymxpc2hQcm9taXNlcy5kZWxldGUobWVkaWFTdHJlYW1JZCk7XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICB9KTtcbiAgICB9XG4gIH1cblxuICAvKipcbiAgICogQGZ1bmN0aW9uIF90cmFja3NSZW1vdmVkSGFuZGxlclxuICAgKiBAZGVzYyBIYW5kbGUgdHJhY2sgcmVtb3ZlZCBldmVudCBmcm9tIHJlbW90ZSBzaWRlLlxuICAgKiBAcHJpdmF0ZVxuICAgKi9cbiAgX3RyYWNrc1JlbW92ZWRIYW5kbGVyKGlkcykge1xuICAgIC8vIEN1cnJlbnRseSwgfGlkc3wgY29udGFpbnMgYWxsIHRyYWNrIElEcyBvZiBhIE1lZGlhU3RyZWFtLiBGb2xsb3dpbmcgYWxnb3JpdGhtIGFsc28gaGFuZGxlcyB8aWRzfCBpcyBhIHBhcnQgb2YgYSBNZWRpYVN0cmVhbSdzIHRyYWNrcy5cbiAgICBmb3IgKGNvbnN0IGlkIG9mIGlkcykge1xuICAgICAgLy8gSXQgY291bGQgYmUgYSBwcm9ibGVtIGlmIHRoZXJlIGlzIGEgdHJhY2sgcHVibGlzaGVkIHdpdGggZGlmZmVyZW50IE1lZGlhU3RyZWFtcy5cbiAgICAgIHRoaXMuX3B1Ymxpc2hlZFN0cmVhbVRyYWNrcy5mb3JFYWNoKChtZWRpYVRyYWNrSWRzLCBtZWRpYVN0cmVhbUlkKSA9PiB7XG4gICAgICAgIGZvciAobGV0IGkgPSAwOyBpIDwgbWVkaWFUcmFja0lkcy5sZW5ndGg7IGkrKykge1xuICAgICAgICAgIGlmIChtZWRpYVRyYWNrSWRzW2ldID09PSBpZCkge1xuICAgICAgICAgICAgbWVkaWFUcmFja0lkcy5zcGxpY2UoaSwgMSk7XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICB9KTtcbiAgICB9XG4gIH1cblxuICAvKipcbiAgICogQGZ1bmN0aW9uIF9kYXRhUmVjZWl2ZWRIYW5kbGVyXG4gICAqIEBkZXNjIEhhbmRsZSBkYXRhIHJlY2VpdmVkIGV2ZW50IGZyb20gcmVtb3RlIHNpZGUuXG4gICAqIEBwcml2YXRlXG4gICAqL1xuICBfZGF0YVJlY2VpdmVkSGFuZGxlcihpZCkge1xuICAgIGlmICghdGhpcy5fc2VuZERhdGFQcm9taXNlcy5oYXMoaWQpKSB7XG4gICAgICBMb2dnZXIud2FybmluZygnUmVjZWl2ZWQgdW5rbm93biBkYXRhIHJlY2VpdmVkIG1lc3NhZ2UuIElEOiAnICsgaWQpO1xuICAgICAgcmV0dXJuO1xuICAgIH0gZWxzZSB7XG4gICAgICB0aGlzLl9zZW5kRGF0YVByb21pc2VzLmdldChpZCkucmVzb2x2ZSgpO1xuICAgIH1cbiAgfVxuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gX3NkcEhhbmRsZXJcbiAgICogQGRlc2MgSGFuZGxlIFNEUCByZWNlaXZlZCBldmVudCBmcm9tIHJlbW90ZSBzaWRlLlxuICAgKiBAcHJpdmF0ZVxuICAgKi9cbiAgX3NkcEhhbmRsZXIoc2RwKSB7XG4gICAgaWYgKHNkcC50eXBlID09PSAnb2ZmZXInKSB7XG4gICAgICB0aGlzLl9vbk9mZmVyKHNkcCk7XG4gICAgfSBlbHNlIGlmIChzZHAudHlwZSA9PT0gJ2Fuc3dlcicpIHtcbiAgICAgIHRoaXMuX29uQW5zd2VyKHNkcCk7XG4gICAgfSBlbHNlIGlmIChzZHAudHlwZSA9PT0gJ2NhbmRpZGF0ZXMnKSB7XG4gICAgICB0aGlzLl9vblJlbW90ZUljZUNhbmRpZGF0ZShzZHApO1xuICAgIH1cbiAgfVxuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gX3RyYWNrU291cmNlc0hhbmRsZXJcbiAgICogQGRlc2MgUmVjZWl2ZWQgdHJhY2sgc291cmNlIGluZm9ybWF0aW9uIGZyb20gcmVtb3RlIHNpZGUuXG4gICAqIEBwcml2YXRlXG4gICAqL1xuICBfdHJhY2tTb3VyY2VzSGFuZGxlcihkYXRhKSB7XG4gICAgZm9yIChjb25zdCBpbmZvIG9mIGRhdGEpIHtcbiAgICAgIHRoaXMuX3JlbW90ZVRyYWNrU291cmNlSW5mby5zZXQoaW5mby5pZCwgaW5mby5zb3VyY2UpO1xuICAgIH1cbiAgfVxuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gX3N0cmVhbUluZm9IYW5kbGVyXG4gICAqIEBkZXNjIFJlY2VpdmVkIHN0cmVhbSBpbmZvcm1hdGlvbiBmcm9tIHJlbW90ZSBzaWRlLlxuICAgKiBAcHJpdmF0ZVxuICAgKi9cbiAgX3N0cmVhbUluZm9IYW5kbGVyKGRhdGEpIHtcbiAgICBpZiAoIWRhdGEpIHtcbiAgICAgIExvZ2dlci53YXJuaW5nKCdVbmV4cGVjdGVkIHN0cmVhbSBpbmZvLicpO1xuICAgICAgcmV0dXJuO1xuICAgIH1cbiAgICB0aGlzLl9yZW1vdGVTdHJlYW1JbmZvLnNldChkYXRhLmlkLCB7XG4gICAgICBzb3VyY2U6IGRhdGEuc291cmNlLFxuICAgICAgYXR0cmlidXRlczogZGF0YS5hdHRyaWJ1dGVzLFxuICAgICAgc3RyZWFtOiBudWxsLFxuICAgICAgbWVkaWFTdHJlYW06IG51bGwsXG4gICAgICB0cmFja0lkczogZGF0YS50cmFja3MsIC8vIFRyYWNrIElEcyBtYXkgbm90IG1hdGNoIGF0IHNlbmRlciBhbmQgcmVjZWl2ZXIgc2lkZXMuIEtlZXAgaXQgZm9yIGxlZ2FjeSBwb3Jwb3Nlcy5cbiAgICB9KTtcbiAgfVxuXG4gIC8qKlxuICAgKiBAZnVuY3Rpb24gX2NoYXRDbG9zZWRIYW5kbGVyXG4gICAqIEBkZXNjIFJlY2VpdmVkIGNoYXQgY2xvc2VkIGV2ZW50IGZyb20gcmVtb3RlIHNpZGUuXG4gICAqIEBwcml2YXRlXG4gICAqL1xuICBfY2hhdENsb3NlZEhhbmRsZXIoZGF0YSkge1xuICAgIHRoaXMuX2Rpc3Bvc2VkID0gdHJ1ZTtcbiAgICB0aGlzLl9zdG9wKGRhdGEsIGZhbHNlKTtcbiAgfVxuXG4gIF9vbk9mZmVyKHNkcCkge1xuICAgIExvZ2dlci5kZWJ1ZygnQWJvdXQgdG8gc2V0IHJlbW90ZSBkZXNjcmlwdGlvbi4gU2lnbmFsaW5nIHN0YXRlOiAnICtcbiAgICAgIHRoaXMuX3BjLnNpZ25hbGluZ1N0YXRlKTtcbiAgICBpZiAodGhpcy5fcGMuc2lnbmFsaW5nU3RhdGUgIT09ICdzdGFibGUnIHx8IHRoaXMuX3NldHRpbmdMb2NhbFNkcCkge1xuICAgICAgaWYgKHRoaXMuX2lzUG9saXRlUGVlcikge1xuICAgICAgICBMb2dnZXIuZGVidWcoJ1JvbGxiYWNrLicpO1xuICAgICAgICB0aGlzLl9zZXR0aW5nTG9jYWxTZHAgPSB0cnVlO1xuICAgICAgICAvLyBzZXRMb2NhbERlc2NyaXB0aW9uKHJvbGxiYWNrKSBpcyBub3Qgc3VwcG9ydGVkIG9uIFNhZmFyaSByaWdodCBub3cuXG4gICAgICAgIC8vIFRlc3QgY2FzZSBcIldlYlJUQyBjb2xsaXNpb24gc2hvdWxkIGJlIHJlc29sdmVkLlwiIGlzIGV4cGVjdGVkIHRvIGZhaWwuXG4gICAgICAgIC8vIFNlZVxuICAgICAgICAvLyBodHRwczovL3dwdC5meWkvcmVzdWx0cy93ZWJydGMvUlRDUGVlckNvbm5lY3Rpb24tc2V0TG9jYWxEZXNjcmlwdGlvbi1yb2xsYmFjay5odG1sP3E9d2VicnRjJnJ1bl9pZD01NjYyMDYyMzIxNTk4NDY0JnJ1bl9pZD01NzU2MTM5NTIwMTMxMDcyJnJ1bl9pZD01NzU0NjM3NTU2NjQ1ODg4JnJ1bl9pZD01NzY0MzM0MDQ5Mjk2Mzg0LlxuICAgICAgICB0aGlzLl9wYy5zZXRMb2NhbERlc2NyaXB0aW9uKCkudGhlbigoKSA9PiB7XG4gICAgICAgICAgdGhpcy5fc2V0dGluZ0xvY2FsU2RwID0gZmFsc2U7XG4gICAgICAgIH0pO1xuICAgICAgfSBlbHNlIHtcbiAgICAgICAgTG9nZ2VyLmRlYnVnKCdDb2xsaXNpb24gZGV0ZWN0ZWQuIElnbm9yZSB0aGlzIG9mZmVyLicpO1xuICAgICAgICByZXR1cm47XG4gICAgICB9XG4gICAgfVxuICAgIHNkcC5zZHAgPSB0aGlzLl9zZXRSdHBTZW5kZXJPcHRpb25zKHNkcC5zZHAsIHRoaXMuX2NvbmZpZyk7XG4gICAgY29uc3Qgc2Vzc2lvbkRlc2NyaXB0aW9uID0gbmV3IFJUQ1Nlc3Npb25EZXNjcmlwdGlvbihzZHApO1xuICAgIHRoaXMuX3NldHRpbmdSZW1vdGVTZHAgPSB0cnVlO1xuICAgIHRoaXMuX3BjLnNldFJlbW90ZURlc2NyaXB0aW9uKHNlc3Npb25EZXNjcmlwdGlvbikudGhlbigoKSA9PiB7XG4gICAgICB0aGlzLl9zZXR0aW5nUmVtb3RlU2RwID0gZmFsc2U7XG4gICAgICB0aGlzLl9jcmVhdGVBbmRTZW5kQW5zd2VyKCk7XG4gICAgfSwgKGVycm9yKSA9PiB7XG4gICAgICBMb2dnZXIuZGVidWcoJ1NldCByZW1vdGUgZGVzY3JpcHRpb24gZmFpbGVkLiBNZXNzYWdlOiAnICsgZXJyb3IubWVzc2FnZSk7XG4gICAgICB0aGlzLl9zdG9wKGVycm9yLCB0cnVlKTtcbiAgICB9KTtcbiAgfVxuXG4gIF9vbkFuc3dlcihzZHApIHtcbiAgICBMb2dnZXIuZGVidWcoJ0Fib3V0IHRvIHNldCByZW1vdGUgZGVzY3JpcHRpb24uIFNpZ25hbGluZyBzdGF0ZTogJyArXG4gICAgICB0aGlzLl9wYy5zaWduYWxpbmdTdGF0ZSk7XG4gICAgc2RwLnNkcCA9IHRoaXMuX3NldFJ0cFNlbmRlck9wdGlvbnMoc2RwLnNkcCwgdGhpcy5fY29uZmlnKTtcbiAgICBjb25zdCBzZXNzaW9uRGVzY3JpcHRpb24gPSBuZXcgUlRDU2Vzc2lvbkRlc2NyaXB0aW9uKHNkcCk7XG4gICAgdGhpcy5fc2V0dGluZ1JlbW90ZVNkcCA9IHRydWU7XG4gICAgdGhpcy5fcGMuc2V0UmVtb3RlRGVzY3JpcHRpb24obmV3IFJUQ1Nlc3Npb25EZXNjcmlwdGlvbihcbiAgICAgICAgc2Vzc2lvbkRlc2NyaXB0aW9uKSkudGhlbigoKSA9PiB7XG4gICAgICBMb2dnZXIuZGVidWcoJ1NldCByZW1vdGUgZGVzY3JpcGl0b24gc3VjY2Vzc2Z1bGx5LicpO1xuICAgICAgdGhpcy5fc2V0dGluZ1JlbW90ZVNkcCA9IGZhbHNlO1xuICAgICAgdGhpcy5fZHJhaW5QZW5kaW5nTWVzc2FnZXMoKTtcbiAgICB9LCAoZXJyb3IpID0+IHtcbiAgICAgIExvZ2dlci5kZWJ1ZygnU2V0IHJlbW90ZSBkZXNjcmlwdGlvbiBmYWlsZWQuIE1lc3NhZ2U6ICcgKyBlcnJvci5tZXNzYWdlKTtcbiAgICAgIHRoaXMuX3N0b3AoZXJyb3IsIHRydWUpO1xuICAgIH0pO1xuICB9XG5cbiAgX29uTG9jYWxJY2VDYW5kaWRhdGUoZXZlbnQpIHtcbiAgICBpZiAoZXZlbnQuY2FuZGlkYXRlKSB7XG4gICAgICB0aGlzLl9zZW5kU2RwKHtcbiAgICAgICAgdHlwZTogJ2NhbmRpZGF0ZXMnLFxuICAgICAgICBjYW5kaWRhdGU6IGV2ZW50LmNhbmRpZGF0ZS5jYW5kaWRhdGUsXG4gICAgICAgIHNkcE1pZDogZXZlbnQuY2FuZGlkYXRlLnNkcE1pZCxcbiAgICAgICAgc2RwTUxpbmVJbmRleDogZXZlbnQuY2FuZGlkYXRlLnNkcE1MaW5lSW5kZXgsXG4gICAgICB9KS5jYXRjaCgoZSk9PntcbiAgICAgICAgTG9nZ2VyLndhcm5pbmcoJ0ZhaWxlZCB0byBzZW5kIGNhbmRpZGF0ZS4nKTtcbiAgICAgIH0pO1xuICAgIH0gZWxzZSB7XG4gICAgICBMb2dnZXIuZGVidWcoJ0VtcHR5IGNhbmRpZGF0ZS4nKTtcbiAgICB9XG4gIH1cblxuICBfb25SZW1vdGVUcmFja0FkZGVkKGV2ZW50KSB7XG4gICAgTG9nZ2VyLmRlYnVnKCdSZW1vdGUgdHJhY2sgYWRkZWQuJyk7XG4gICAgZm9yIChjb25zdCBzdHJlYW0gb2YgZXZlbnQuc3RyZWFtcykge1xuICAgICAgaWYgKCF0aGlzLl9yZW1vdGVTdHJlYW1JbmZvLmhhcyhzdHJlYW0uaWQpKSB7XG4gICAgICAgIExvZ2dlci53YXJuaW5nKCdNaXNzaW5nIHN0cmVhbSBpbmZvLicpO1xuICAgICAgICByZXR1cm47XG4gICAgICB9XG4gICAgICBpZiAoIXRoaXMuX3JlbW90ZVN0cmVhbUluZm8uZ2V0KHN0cmVhbS5pZCkuc3RyZWFtKSB7XG4gICAgICAgIHRoaXMuX3NldFN0cmVhbVRvUmVtb3RlU3RyZWFtSW5mbyhzdHJlYW0pO1xuICAgICAgfVxuICAgIH1cbiAgICBpZiAodGhpcy5fcGMuaWNlQ29ubmVjdGlvblN0YXRlID09PSAnY29ubmVjdGVkJyB8fFxuICAgICAgIHRoaXMuX3BjLmljZUNvbm5lY3Rpb25TdGF0ZSA9PT0gJ2NvbXBsZXRlZCcpIHtcbiAgICAgIHRoaXMuX2NoZWNrSWNlQ29ubmVjdGlvblN0YXRlQW5kRmlyZUV2ZW50KCk7XG4gICAgfSBlbHNlIHtcbiAgICAgIHRoaXMuX2FkZGVkVHJhY2tJZHMuY29uY2F0KGV2ZW50LnRyYWNrLmlkKTtcbiAgICB9XG4gIH1cblxuICBfb25SZW1vdGVTdHJlYW1BZGRlZChldmVudCkge1xuICAgIExvZ2dlci5kZWJ1ZygnUmVtb3RlIHN0cmVhbSBhZGRlZC4nKTtcbiAgICBpZiAoIXRoaXMuX3JlbW90ZVN0cmVhbUluZm8uaGFzKGV2ZW50LnN0cmVhbS5pZCkpIHtcbiAgICAgIExvZ2dlci53YXJuaW5nKCdDYW5ub3QgZmluZCBzb3VyY2UgaW5mbyBmb3Igc3RyZWFtICcgKyBldmVudC5zdHJlYW0uaWQpO1xuICAgICAgcmV0dXJuO1xuICAgIH1cbiAgICBpZiAodGhpcy5fcGMuaWNlQ29ubmVjdGlvblN0YXRlID09PSAnY29ubmVjdGVkJyB8fFxuICAgICAgdGhpcy5fcGMuaWNlQ29ubmVjdGlvblN0YXRlID09PSAnY29tcGxldGVkJykge1xuICAgICAgdGhpcy5fc2VuZFNpZ25hbGluZ01lc3NhZ2UoU2lnbmFsaW5nVHlwZS5UUkFDS1NfQURERUQsXG4gICAgICAgICAgdGhpcy5fcmVtb3RlU3RyZWFtSW5mby5nZXQoZXZlbnQuc3RyZWFtLmlkKS50cmFja0lkcyk7XG4gICAgfSBlbHNlIHtcbiAgICAgIHRoaXMuX2FkZGVkVHJhY2tJZHMgPSB0aGlzLl9hZGRlZFRyYWNrSWRzLmNvbmNhdChcbiAgICAgICAgICB0aGlzLl9yZW1vdGVTdHJlYW1JbmZvLmdldChldmVudC5zdHJlYW0uaWQpLnRyYWNrSWRzKTtcbiAgICB9XG4gICAgY29uc3QgYXVkaW9UcmFja1NvdXJjZSA9IHRoaXMuX3JlbW90ZVN0cmVhbUluZm8uZ2V0KGV2ZW50LnN0cmVhbS5pZClcbiAgICAgICAgLnNvdXJjZS5hdWRpbztcbiAgICBjb25zdCB2aWRlb1RyYWNrU291cmNlID0gdGhpcy5fcmVtb3RlU3RyZWFtSW5mby5nZXQoZXZlbnQuc3RyZWFtLmlkKVxuICAgICAgICAuc291cmNlLnZpZGVvO1xuICAgIGNvbnN0IHNvdXJjZUluZm8gPSBuZXcgU3RyZWFtTW9kdWxlLlN0cmVhbVNvdXJjZUluZm8oYXVkaW9UcmFja1NvdXJjZSxcbiAgICAgICAgdmlkZW9UcmFja1NvdXJjZSk7XG4gICAgaWYgKFV0aWxzLmlzU2FmYXJpKCkpIHtcbiAgICAgIGlmICghc291cmNlSW5mby5hdWRpbykge1xuICAgICAgICBldmVudC5zdHJlYW0uZ2V0QXVkaW9UcmFja3MoKS5mb3JFYWNoKCh0cmFjaykgPT4ge1xuICAgICAgICAgIGV2ZW50LnN0cmVhbS5yZW1vdmVUcmFjayh0cmFjayk7XG4gICAgICAgIH0pO1xuICAgICAgfVxuICAgICAgaWYgKCFzb3VyY2VJbmZvLnZpZGVvKSB7XG4gICAgICAgIGV2ZW50LnN0cmVhbS5nZXRWaWRlb1RyYWNrcygpLmZvckVhY2goKHRyYWNrKSA9PiB7XG4gICAgICAgICAgZXZlbnQuc3RyZWFtLnJlbW92ZVRyYWNrKHRyYWNrKTtcbiAgICAgICAgfSk7XG4gICAgICB9XG4gICAgfVxuICAgIGNvbnN0IGF0dHJpYnV0ZXMgPSB0aGlzLl9yZW1vdGVTdHJlYW1JbmZvLmdldChldmVudC5zdHJlYW0uaWQpLmF0dHJpYnV0ZXM7XG4gICAgY29uc3Qgc3RyZWFtID0gbmV3IFN0cmVhbU1vZHVsZS5SZW1vdGVTdHJlYW0odW5kZWZpbmVkLCB0aGlzLl9yZW1vdGVJZCxcbiAgICAgICAgZXZlbnQuc3RyZWFtLCBzb3VyY2VJbmZvLCBhdHRyaWJ1dGVzKTtcbiAgICBpZiAoc3RyZWFtKSB7XG4gICAgICB0aGlzLl9yZW1vdGVTdHJlYW1zLnB1c2goc3RyZWFtKTtcbiAgICAgIGNvbnN0IHN0cmVhbUV2ZW50ID0gbmV3IFN0cmVhbU1vZHVsZS5TdHJlYW1FdmVudCgnc3RyZWFtYWRkZWQnLCB7XG4gICAgICAgIHN0cmVhbTogc3RyZWFtLFxuICAgICAgfSk7XG4gICAgICB0aGlzLmRpc3BhdGNoRXZlbnQoc3RyZWFtRXZlbnQpO1xuICAgIH1cbiAgfVxuXG4gIF9vblJlbW90ZVN0cmVhbVJlbW92ZWQoZXZlbnQpIHtcbiAgICBMb2dnZXIuZGVidWcoJ1JlbW90ZSBzdHJlYW0gcmVtb3ZlZC4nKTtcbiAgICBjb25zdCBpID0gdGhpcy5fcmVtb3RlU3RyZWFtcy5maW5kSW5kZXgoKHMpID0+IHtcbiAgICAgIHJldHVybiBzLm1lZGlhU3RyZWFtLmlkID09PSBldmVudC5zdHJlYW0uaWQ7XG4gICAgfSk7XG4gICAgaWYgKGkgIT09IC0xKSB7XG4gICAgICBjb25zdCBzdHJlYW0gPSB0aGlzLl9yZW1vdGVTdHJlYW1zW2ldO1xuICAgICAgdGhpcy5fc3RyZWFtUmVtb3ZlZChzdHJlYW0pO1xuICAgICAgdGhpcy5fcmVtb3RlU3RyZWFtcy5zcGxpY2UoaSwgMSk7XG4gICAgfVxuICB9XG5cbiAgX29uTmVnb3RpYXRpb25uZWVkZWQoKSB7XG4gICAgaWYgKHRoaXMuX3BjLnNpZ25hbGluZ1N0YXRlID09PSAnc3RhYmxlJyAmJiAhdGhpcy5fcGMuX3NldHRpbmdMb2NhbFNkcCAmJlxuICAgICAgICAhdGhpcy5fc2V0dGluZ1JlbW90ZVNkcCkge1xuICAgICAgdGhpcy5fZG9OZWdvdGlhdGUoKTtcbiAgICB9IGVsc2Uge1xuICAgICAgdGhpcy5faXNOZWdvdGlhdGlvbk5lZWRlZCA9IHRydWU7XG4gICAgfVxuICB9XG5cbiAgX29uUmVtb3RlSWNlQ2FuZGlkYXRlKGNhbmRpZGF0ZUluZm8pIHtcbiAgICBjb25zdCBjYW5kaWRhdGUgPSBuZXcgUlRDSWNlQ2FuZGlkYXRlKHtcbiAgICAgIGNhbmRpZGF0ZTogY2FuZGlkYXRlSW5mby5jYW5kaWRhdGUsXG4gICAgICBzZHBNaWQ6IGNhbmRpZGF0ZUluZm8uc2RwTWlkLFxuICAgICAgc2RwTUxpbmVJbmRleDogY2FuZGlkYXRlSW5mby5zZHBNTGluZUluZGV4LFxuICAgIH0pO1xuICAgIGlmICh0aGlzLl9wYy5yZW1vdGVEZXNjcmlwdGlvbiAmJiB0aGlzLl9wYy5yZW1vdGVEZXNjcmlwdGlvbi5zZHAgIT09ICcnKSB7XG4gICAgICBMb2dnZXIuZGVidWcoJ0FkZCByZW1vdGUgaWNlIGNhbmRpZGF0ZXMuJyk7XG4gICAgICB0aGlzLl9wYy5hZGRJY2VDYW5kaWRhdGUoY2FuZGlkYXRlKS5jYXRjaCgoZXJyb3IpID0+IHtcbiAgICAgICAgTG9nZ2VyLndhcm5pbmcoJ0Vycm9yIHByb2Nlc3NpbmcgSUNFIGNhbmRpZGF0ZTogJyArIGVycm9yKTtcbiAgICAgIH0pO1xuICAgIH0gZWxzZSB7XG4gICAgICBMb2dnZXIuZGVidWcoJ0NhY2hlIHJlbW90ZSBpY2UgY2FuZGlkYXRlcy4nKTtcbiAgICAgIHRoaXMuX3JlbW90ZUljZUNhbmRpZGF0ZXMucHVzaChjYW5kaWRhdGUpO1xuICAgIH1cbiAgfVxuXG4gIF9vblNpZ25hbGluZ1N0YXRlQ2hhbmdlKGV2ZW50KSB7XG4gICAgTG9nZ2VyLmRlYnVnKCdTaWduYWxpbmcgc3RhdGUgY2hhbmdlZDogJyArIHRoaXMuX3BjLnNpZ25hbGluZ1N0YXRlKTtcbiAgICBpZiAodGhpcy5fcGMuc2lnbmFsaW5nU3RhdGUgPT09ICdoYXZlLXJlbW90ZS1vZmZlcicgfHxcbiAgICAgICAgdGhpcy5fcGMuc2lnbmFsaW5nU3RhdGUgPT09ICdzdGFibGUnKSB7XG4gICAgICB0aGlzLl9kcmFpblBlbmRpbmdSZW1vdGVJY2VDYW5kaWRhdGVzKCk7XG4gICAgfVxuICAgIGlmICh0aGlzLl9wYy5zaWduYWxpbmdTdGF0ZSA9PT0gJ3N0YWJsZScpIHtcbiAgICAgIHRoaXMuX25lZ290aWF0aW5nID0gZmFsc2U7XG4gICAgICBpZiAodGhpcy5faXNOZWdvdGlhdGlvbk5lZWRlZCkge1xuICAgICAgICB0aGlzLl9vbk5lZ290aWF0aW9ubmVlZGVkKCk7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICB0aGlzLl9kcmFpblBlbmRpbmdTdHJlYW1zKCk7XG4gICAgICAgIHRoaXMuX2RyYWluUGVuZGluZ01lc3NhZ2VzKCk7XG4gICAgICB9XG4gICAgfVxuICB9XG5cbiAgX29uSWNlQ29ubmVjdGlvblN0YXRlQ2hhbmdlKGV2ZW50KSB7XG4gICAgaWYgKGV2ZW50LmN1cnJlbnRUYXJnZXQuaWNlQ29ubmVjdGlvblN0YXRlID09PSAnY2xvc2VkJyB8fFxuICAgICAgICBldmVudC5jdXJyZW50VGFyZ2V0LmljZUNvbm5lY3Rpb25TdGF0ZSA9PT0gJ2ZhaWxlZCcpIHtcbiAgICAgIGNvbnN0IGVycm9yID0gbmV3IEVycm9yTW9kdWxlLlAyUEVycm9yKFxuICAgICAgICAgIEVycm9yTW9kdWxlLmVycm9ycy5QMlBfV0VCUlRDX1VOS05PV04sXG4gICAgICAgICAgJ0lDRSBjb25uZWN0aW9uIGZhaWxlZCBvciBjbG9zZWQuJyk7XG4gICAgICB0aGlzLl9zdG9wKGVycm9yLCB0cnVlKTtcbiAgICB9IGVsc2UgaWYgKGV2ZW50LmN1cnJlbnRUYXJnZXQuaWNlQ29ubmVjdGlvblN0YXRlID09PSAnY29ubmVjdGVkJyB8fFxuICAgICAgZXZlbnQuY3VycmVudFRhcmdldC5pY2VDb25uZWN0aW9uU3RhdGUgPT09ICdjb21wbGV0ZWQnKSB7XG4gICAgICB0aGlzLl9zZW5kU2lnbmFsaW5nTWVzc2FnZShTaWduYWxpbmdUeXBlLlRSQUNLU19BRERFRCxcbiAgICAgICAgICB0aGlzLl9hZGRlZFRyYWNrSWRzKTtcbiAgICAgIHRoaXMuX2FkZGVkVHJhY2tJZHMgPSBbXTtcbiAgICAgIHRoaXMuX2NoZWNrSWNlQ29ubmVjdGlvblN0YXRlQW5kRmlyZUV2ZW50KCk7XG4gICAgfVxuICB9XG5cbiAgX29uRGF0YUNoYW5uZWxNZXNzYWdlKGV2ZW50KSB7XG4gICAgY29uc3QgbWVzc2FnZT1KU09OLnBhcnNlKGV2ZW50LmRhdGEpO1xuICAgIGlmICghdGhpcy5fcmVtb3RlU2lkZUlnbm9yZXNEYXRhQ2hhbm5lbEFja3MpIHtcbiAgICAgIHRoaXMuX3NlbmRTaWduYWxpbmdNZXNzYWdlKFNpZ25hbGluZ1R5cGUuREFUQV9SRUNFSVZFRCwgbWVzc2FnZS5pZCk7XG4gICAgfVxuICAgIGNvbnN0IG1lc3NhZ2VFdmVudCA9IG5ldyBNZXNzYWdlRXZlbnQoJ21lc3NhZ2VyZWNlaXZlZCcsIHtcbiAgICAgIG1lc3NhZ2U6IG1lc3NhZ2UuZGF0YSxcbiAgICAgIG9yaWdpbjogdGhpcy5fcmVtb3RlSWQsXG4gICAgfSk7XG4gICAgdGhpcy5kaXNwYXRjaEV2ZW50KG1lc3NhZ2VFdmVudCk7XG4gIH1cblxuICBfb25EYXRhQ2hhbm5lbE9wZW4oZXZlbnQpIHtcbiAgICBMb2dnZXIuZGVidWcoJ0RhdGEgQ2hhbm5lbCBpcyBvcGVuZWQuJyk7XG4gICAgaWYgKGV2ZW50LnRhcmdldC5sYWJlbCA9PT0gRGF0YUNoYW5uZWxMYWJlbC5NRVNTQUdFKSB7XG4gICAgICBMb2dnZXIuZGVidWcoJ0RhdGEgY2hhbm5lbCBmb3IgbWVzc2FnZXMgaXMgb3BlbmVkLicpO1xuICAgICAgdGhpcy5fZHJhaW5QZW5kaW5nTWVzc2FnZXMoKTtcbiAgICB9XG4gIH1cblxuICBfb25EYXRhQ2hhbm5lbENsb3NlKGV2ZW50KSB7XG4gICAgTG9nZ2VyLmRlYnVnKCdEYXRhIENoYW5uZWwgaXMgY2xvc2VkLicpO1xuICB9XG5cbiAgX3N0cmVhbVJlbW92ZWQoc3RyZWFtKSB7XG4gICAgaWYgKCF0aGlzLl9yZW1vdGVTdHJlYW1JbmZvLmhhcyhzdHJlYW0ubWVkaWFTdHJlYW0uaWQpKSB7XG4gICAgICBMb2dnZXIud2FybmluZygnQ2Fubm90IGZpbmQgc3RyZWFtIGluZm8uJyk7XG4gICAgfVxuICAgIHRoaXMuX3NlbmRTaWduYWxpbmdNZXNzYWdlKFNpZ25hbGluZ1R5cGUuVFJBQ0tTX1JFTU9WRUQsXG4gICAgICAgIHRoaXMuX3JlbW90ZVN0cmVhbUluZm8uZ2V0KHN0cmVhbS5tZWRpYVN0cmVhbS5pZCkudHJhY2tJZHMpO1xuICAgIGNvbnN0IGV2ZW50ID0gbmV3IE93dEV2ZW50KCdlbmRlZCcpO1xuICAgIHN0cmVhbS5kaXNwYXRjaEV2ZW50KGV2ZW50KTtcbiAgfVxuXG4gIF9jcmVhdGVQZWVyQ29ubmVjdGlvbigpIHtcbiAgICBjb25zdCBwY0NvbmZpZ3VyYXRpb24gPSB0aGlzLl9jb25maWcucnRjQ29uZmlndXJhdGlvbiB8fCB7fTtcbiAgICB0aGlzLl9wYyA9IG5ldyBSVENQZWVyQ29ubmVjdGlvbihwY0NvbmZpZ3VyYXRpb24pO1xuICAgIHRoaXMuX3BjLm9udHJhY2sgPSAoZXZlbnQpID0+IHtcbiAgICAgIHRoaXMuX29uUmVtb3RlVHJhY2tBZGRlZC5hcHBseSh0aGlzLCBbZXZlbnRdKTtcbiAgICB9O1xuICAgIHRoaXMuX3BjLm9uaWNlY2FuZGlkYXRlID0gKGV2ZW50KSA9PiB7XG4gICAgICB0aGlzLl9vbkxvY2FsSWNlQ2FuZGlkYXRlLmFwcGx5KHRoaXMsIFtldmVudF0pO1xuICAgIH07XG4gICAgdGhpcy5fcGMub25zaWduYWxpbmdzdGF0ZWNoYW5nZSA9IChldmVudCkgPT4ge1xuICAgICAgdGhpcy5fb25TaWduYWxpbmdTdGF0ZUNoYW5nZS5hcHBseSh0aGlzLCBbZXZlbnRdKTtcbiAgICB9O1xuICAgIHRoaXMuX3BjLm9uZGF0YWNoYW5uZWwgPSAoZXZlbnQpID0+IHtcbiAgICAgIExvZ2dlci5kZWJ1ZygnT24gZGF0YSBjaGFubmVsLicpO1xuICAgICAgLy8gU2F2ZSByZW1vdGUgY3JlYXRlZCBkYXRhIGNoYW5uZWwuXG4gICAgICBpZiAoIXRoaXMuX2RhdGFDaGFubmVscy5oYXMoZXZlbnQuY2hhbm5lbC5sYWJlbCkpIHtcbiAgICAgICAgdGhpcy5fZGF0YUNoYW5uZWxzLnNldChldmVudC5jaGFubmVsLmxhYmVsLCBldmVudC5jaGFubmVsKTtcbiAgICAgICAgTG9nZ2VyLmRlYnVnKCdTYXZlIHJlbW90ZSBjcmVhdGVkIGRhdGEgY2hhbm5lbC4nKTtcbiAgICAgIH1cbiAgICAgIHRoaXMuX2JpbmRFdmVudHNUb0RhdGFDaGFubmVsKGV2ZW50LmNoYW5uZWwpO1xuICAgIH07XG4gICAgdGhpcy5fcGMub25pY2Vjb25uZWN0aW9uc3RhdGVjaGFuZ2UgPSAoZXZlbnQpID0+IHtcbiAgICAgIHRoaXMuX29uSWNlQ29ubmVjdGlvblN0YXRlQ2hhbmdlLmFwcGx5KHRoaXMsIFtldmVudF0pO1xuICAgIH07XG4gICAgdGhpcy5fcGMub25uZWdvdGlhdGlvbm5lZWRlZCA9ICgpID0+IHtcbiAgICAgIHRoaXMuX29uTmVnb3RpYXRpb25uZWVkZWQoKTtcbiAgICB9O1xuICB9XG5cbiAgX2RyYWluUGVuZGluZ1N0cmVhbXMoKSB7XG4gICAgTG9nZ2VyLmRlYnVnKCdEcmFpbmluZyBwZW5kaW5nIHN0cmVhbXMuJyk7XG4gICAgaWYgKHRoaXMuX3BjICYmIHRoaXMuX3BjLnNpZ25hbGluZ1N0YXRlID09PSAnc3RhYmxlJykge1xuICAgICAgTG9nZ2VyLmRlYnVnKCdQZWVyIGNvbm5lY3Rpb24gaXMgcmVhZHkgZm9yIGRyYWluaW5nIHBlbmRpbmcgc3RyZWFtcy4nKTtcbiAgICAgIGZvciAobGV0IGkgPSAwOyBpIDwgdGhpcy5fcGVuZGluZ1N0cmVhbXMubGVuZ3RoOyBpKyspIHtcbiAgICAgICAgY29uc3Qgc3RyZWFtID0gdGhpcy5fcGVuZGluZ1N0cmVhbXNbaV07XG4gICAgICAgIHRoaXMuX3BlbmRpbmdTdHJlYW1zLnNoaWZ0KCk7XG4gICAgICAgIGlmICghc3RyZWFtLm1lZGlhU3RyZWFtKSB7XG4gICAgICAgICAgY29udGludWU7XG4gICAgICAgIH1cbiAgICAgICAgdGhpcy5fYWRkU3RyZWFtKHN0cmVhbS5tZWRpYVN0cmVhbSk7XG4gICAgICAgIExvZ2dlci5kZWJ1ZygnQWRkZWQgc3RyZWFtIHRvIHBlZXIgY29ubmVjdGlvbi4nKTtcbiAgICAgICAgdGhpcy5fcHVibGlzaGluZ1N0cmVhbXMucHVzaChzdHJlYW0pO1xuICAgICAgfVxuICAgICAgdGhpcy5fcGVuZGluZ1N0cmVhbXMubGVuZ3RoID0gMDtcbiAgICAgIGZvciAoY29uc3Qgc3RyZWFtIG9mIHRoaXMuX3BlbmRpbmdVbnB1Ymxpc2hTdHJlYW1zKSB7XG4gICAgICAgIGlmICghc3RyZWFtLnN0cmVhbSkge1xuICAgICAgICAgIGNvbnRpbnVlO1xuICAgICAgICB9XG4gICAgICAgIGlmICh0eXBlb2YgdGhpcy5fcGMuZ2V0VHJhbnNjZWl2ZXJzID09PSAnZnVuY3Rpb24nICYmXG4gICAgICAgICAgICB0eXBlb2YgdGhpcy5fcGMucmVtb3ZlVHJhY2sgPT09ICdmdW5jdGlvbicpIHtcbiAgICAgICAgICBmb3IgKGNvbnN0IHRyYW5zY2VpdmVyIG9mIHRoaXMuX3BjLmdldFRyYW5zY2VpdmVycygpKSB7XG4gICAgICAgICAgICBmb3IgKGNvbnN0IHRyYWNrIG9mIHN0cmVhbS5zdHJlYW0uZ2V0VHJhY2tzKCkpIHtcbiAgICAgICAgICAgICAgaWYgKHRyYW5zY2VpdmVyLnNlbmRlci50cmFjayA9PSB0cmFjaykge1xuICAgICAgICAgICAgICAgIGlmICh0cmFuc2NlaXZlci5kaXJlY3Rpb24gPT09ICdzZW5kb25seScpIHtcbiAgICAgICAgICAgICAgICAgIHRyYW5zY2VpdmVyLnN0b3AoKTtcbiAgICAgICAgICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgICAgICAgICAgdGhpcy5fcGMucmVtb3ZlVHJhY2sodHJhY2spO1xuICAgICAgICAgICAgICAgIH1cbiAgICAgICAgICAgICAgfVxuICAgICAgICAgICAgfVxuICAgICAgICAgIH1cbiAgICAgICAgfSBlbHNlIHtcbiAgICAgICAgICBMb2dnZXIuZGVidWcoXG4gICAgICAgICAgICAgICdnZXRTZW5kZXIgb3IgcmVtb3ZlVHJhY2sgaXMgbm90IHN1cHBvcnRlZCwgZmFsbGJhY2sgdG8gJyArXG4gICAgICAgICAgICAgICdyZW1vdmVTdHJlYW0uJyk7XG4gICAgICAgICAgdGhpcy5fcGMucmVtb3ZlU3RyZWFtKHN0cmVhbS5zdHJlYW0pO1xuICAgICAgICB9XG4gICAgICAgIHRoaXMuX3VucHVibGlzaFByb21pc2VzLmdldChzdHJlYW0uc3RyZWFtLmlkKS5yZXNvbHZlKCk7XG4gICAgICAgIHRoaXMuX3B1Ymxpc2hlZFN0cmVhbXMuZGVsZXRlKHN0cmVhbSk7XG4gICAgICB9XG4gICAgICB0aGlzLl9wZW5kaW5nVW5wdWJsaXNoU3RyZWFtcy5sZW5ndGggPSAwO1xuICAgIH1cbiAgfVxuXG4gIF9kcmFpblBlbmRpbmdSZW1vdGVJY2VDYW5kaWRhdGVzKCkge1xuICAgIGZvciAobGV0IGkgPSAwOyBpIDwgdGhpcy5fcmVtb3RlSWNlQ2FuZGlkYXRlcy5sZW5ndGg7IGkrKykge1xuICAgICAgTG9nZ2VyLmRlYnVnKCdBZGQgY2FuZGlkYXRlJyk7XG4gICAgICB0aGlzLl9wYy5hZGRJY2VDYW5kaWRhdGUodGhpcy5fcmVtb3RlSWNlQ2FuZGlkYXRlc1tpXSkuY2F0Y2goKGVycm9yKT0+e1xuICAgICAgICBMb2dnZXIud2FybmluZygnRXJyb3IgcHJvY2Vzc2luZyBJQ0UgY2FuZGlkYXRlOiAnK2Vycm9yKTtcbiAgICAgIH0pO1xuICAgIH1cbiAgICB0aGlzLl9yZW1vdGVJY2VDYW5kaWRhdGVzLmxlbmd0aCA9IDA7XG4gIH1cblxuICBfZHJhaW5QZW5kaW5nTWVzc2FnZXMoKSB7XG4gICAgTG9nZ2VyLmRlYnVnKCdEcmFpbmluZyBwZW5kaW5nIG1lc3NhZ2VzLicpO1xuICAgIGlmICh0aGlzLl9wZW5kaW5nTWVzc2FnZXMubGVuZ3RoID09IDApIHtcbiAgICAgIHJldHVybjtcbiAgICB9XG4gICAgY29uc3QgZGMgPSB0aGlzLl9kYXRhQ2hhbm5lbHMuZ2V0KERhdGFDaGFubmVsTGFiZWwuTUVTU0FHRSk7XG4gICAgaWYgKGRjICYmIGRjLnJlYWR5U3RhdGUgPT09ICdvcGVuJykge1xuICAgICAgZm9yIChsZXQgaSA9IDA7IGkgPCB0aGlzLl9wZW5kaW5nTWVzc2FnZXMubGVuZ3RoOyBpKyspIHtcbiAgICAgICAgTG9nZ2VyLmRlYnVnKFxuICAgICAgICAgICAgJ1NlbmRpbmcgbWVzc2FnZSB2aWEgZGF0YSBjaGFubmVsOiAnICsgdGhpcy5fcGVuZGluZ01lc3NhZ2VzW2ldKTtcbiAgICAgICAgZGMuc2VuZChKU09OLnN0cmluZ2lmeSh0aGlzLl9wZW5kaW5nTWVzc2FnZXNbaV0pKTtcbiAgICAgICAgY29uc3QgbWVzc2FnZUlkID0gdGhpcy5fcGVuZGluZ01lc3NhZ2VzW2ldLmlkO1xuICAgICAgICBpZiAodGhpcy5fc2VuZERhdGFQcm9taXNlcy5oYXMobWVzc2FnZUlkKSkge1xuICAgICAgICAgIHRoaXMuX3NlbmREYXRhUHJvbWlzZXMuZ2V0KG1lc3NhZ2VJZCkucmVzb2x2ZSgpO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgICB0aGlzLl9wZW5kaW5nTWVzc2FnZXMubGVuZ3RoID0gMDtcbiAgICB9IGVsc2UgaWYgKHRoaXMuX3BjICYmIHRoaXMuX3BjLmNvbm5lY3Rpb25TdGF0ZSAhPT0gJ2Nsb3NlZCcgJiYgIWRjKSB7XG4gICAgICB0aGlzLl9jcmVhdGVEYXRhQ2hhbm5lbChEYXRhQ2hhbm5lbExhYmVsLk1FU1NBR0UpO1xuICAgIH1cbiAgfVxuXG4gIF9zZW5kU3RyZWFtSW5mbyhzdHJlYW0pIHtcbiAgICBpZiAoIXN0cmVhbSB8fCAhc3RyZWFtLm1lZGlhU3RyZWFtKSB7XG4gICAgICByZXR1cm4gbmV3IEVycm9yTW9kdWxlLlAyUEVycm9yKFxuICAgICAgICAgIEVycm9yTW9kdWxlLmVycm9ycy5QMlBfQ0xJRU5UX0lMTEVHQUxfQVJHVU1FTlQpO1xuICAgIH1cbiAgICBjb25zdCBpbmZvID0gW107XG4gICAgc3RyZWFtLm1lZGlhU3RyZWFtLmdldFRyYWNrcygpLm1hcCgodHJhY2spID0+IHtcbiAgICAgIGluZm8ucHVzaCh7XG4gICAgICAgIGlkOiB0cmFjay5pZCxcbiAgICAgICAgc291cmNlOiBzdHJlYW0uc291cmNlW3RyYWNrLmtpbmRdLFxuICAgICAgfSk7XG4gICAgfSk7XG4gICAgcmV0dXJuIFByb21pc2UuYWxsKFt0aGlzLl9zZW5kU2lnbmFsaW5nTWVzc2FnZShTaWduYWxpbmdUeXBlLlRSQUNLX1NPVVJDRVMsXG4gICAgICAgIGluZm8pLFxuICAgIHRoaXMuX3NlbmRTaWduYWxpbmdNZXNzYWdlKFNpZ25hbGluZ1R5cGUuU1RSRUFNX0lORk8sIHtcbiAgICAgIGlkOiBzdHJlYW0ubWVkaWFTdHJlYW0uaWQsXG4gICAgICBhdHRyaWJ1dGVzOiBzdHJlYW0uYXR0cmlidXRlcyxcbiAgICAgIC8vIFRyYWNrIElEcyBtYXkgbm90IG1hdGNoIGF0IHNlbmRlciBhbmQgcmVjZWl2ZXIgc2lkZXMuXG4gICAgICB0cmFja3M6IEFycmF5LmZyb20oaW5mbywgKGl0ZW0pID0+IGl0ZW0uaWQpLFxuICAgICAgLy8gVGhpcyBpcyBhIHdvcmthcm91bmQgZm9yIFNhZmFyaS4gUGxlYXNlIHVzZSB0cmFjay1zb3VyY2VzIGlmIHBvc3NpYmxlLlxuICAgICAgc291cmNlOiBzdHJlYW0uc291cmNlLFxuICAgIH0pLFxuICAgIF0pO1xuICB9XG5cbiAgX2hhbmRsZVJlbW90ZUNhcGFiaWxpdHkodWEpIHtcbiAgICBpZiAodWEuc2RrICYmIHVhLnNkayAmJiB1YS5zZGsudHlwZSA9PT0gJ0phdmFTY3JpcHQnICYmIHVhLnJ1bnRpbWUgJiZcbiAgICAgICAgdWEucnVudGltZS5uYW1lID09PSAnRmlyZWZveCcpIHtcbiAgICAgIHRoaXMuX3JlbW90ZVNpZGVTdXBwb3J0c1JlbW92ZVN0cmVhbSA9IGZhbHNlO1xuICAgIH0gZWxzZSB7IC8vIFJlbW90ZSBzaWRlIGlzIGlPUy9BbmRyb2lkL0MrKyB3aGljaCB1c2VzIEdvb2dsZSdzIFdlYlJUQyBzdGFjay5cbiAgICAgIHRoaXMuX3JlbW90ZVNpZGVTdXBwb3J0c1JlbW92ZVN0cmVhbSA9IHRydWU7XG4gICAgfVxuICAgIGlmICh1YS5jYXBhYmlsaXRpZXMpIHtcbiAgICAgIHRoaXMuX3JlbW90ZVNpZGVJZ25vcmVzRGF0YUNoYW5uZWxBY2tzID1cbiAgICAgICAgICB1YS5jYXBhYmlsaXRpZXMuaWdub3JlRGF0YUNoYW5uZWxBY2tzO1xuICAgIH1cbiAgfVxuXG4gIF9kb05lZ290aWF0ZSgpIHtcbiAgICB0aGlzLl9jcmVhdGVBbmRTZW5kT2ZmZXIoKTtcbiAgfVxuXG4gIF9zZXRDb2RlY09yZGVyKHNkcCkge1xuICAgIGlmICh0aGlzLl9jb25maWcuYXVkaW9FbmNvZGluZ3MpIHtcbiAgICAgIGNvbnN0IGF1ZGlvQ29kZWNOYW1lcyA9IEFycmF5LmZyb20odGhpcy5fY29uZmlnLmF1ZGlvRW5jb2RpbmdzLFxuICAgICAgICAgIChlbmNvZGluZ1BhcmFtZXRlcnMpID0+IGVuY29kaW5nUGFyYW1ldGVycy5jb2RlYy5uYW1lKTtcbiAgICAgIHNkcCA9IFNkcFV0aWxzLnJlb3JkZXJDb2RlY3Moc2RwLCAnYXVkaW8nLCBhdWRpb0NvZGVjTmFtZXMpO1xuICAgIH1cbiAgICBpZiAodGhpcy5fY29uZmlnLnZpZGVvRW5jb2RpbmdzKSB7XG4gICAgICBjb25zdCB2aWRlb0NvZGVjTmFtZXMgPSBBcnJheS5mcm9tKHRoaXMuX2NvbmZpZy52aWRlb0VuY29kaW5ncyxcbiAgICAgICAgICAoZW5jb2RpbmdQYXJhbWV0ZXJzKSA9PiBlbmNvZGluZ1BhcmFtZXRlcnMuY29kZWMubmFtZSk7XG4gICAgICBzZHAgPSBTZHBVdGlscy5yZW9yZGVyQ29kZWNzKHNkcCwgJ3ZpZGVvJywgdmlkZW9Db2RlY05hbWVzKTtcbiAgICB9XG4gICAgcmV0dXJuIHNkcDtcbiAgfVxuXG4gIF9zZXRNYXhCaXRyYXRlKHNkcCwgb3B0aW9ucykge1xuICAgIGlmICh0eXBlb2Ygb3B0aW9ucy5hdWRpb0VuY29kaW5ncyA9PT0gJ29iamVjdCcpIHtcbiAgICAgIHNkcCA9IFNkcFV0aWxzLnNldE1heEJpdHJhdGUoc2RwLCBvcHRpb25zLmF1ZGlvRW5jb2RpbmdzKTtcbiAgICB9XG4gICAgaWYgKHR5cGVvZiBvcHRpb25zLnZpZGVvRW5jb2RpbmdzID09PSAnb2JqZWN0Jykge1xuICAgICAgc2RwID0gU2RwVXRpbHMuc2V0TWF4Qml0cmF0ZShzZHAsIG9wdGlvbnMudmlkZW9FbmNvZGluZ3MpO1xuICAgIH1cbiAgICByZXR1cm4gc2RwO1xuICB9XG5cbiAgX3NldFJ0cFNlbmRlck9wdGlvbnMoc2RwLCBvcHRpb25zKSB7XG4gICAgc2RwID0gdGhpcy5fc2V0TWF4Qml0cmF0ZShzZHAsIG9wdGlvbnMpO1xuICAgIHJldHVybiBzZHA7XG4gIH1cblxuICBfc2V0UnRwUmVjZWl2ZXJPcHRpb25zKHNkcCkge1xuICAgIHNkcCA9IHRoaXMuX3NldENvZGVjT3JkZXIoc2RwKTtcbiAgICByZXR1cm4gc2RwO1xuICB9XG5cbiAgX2NyZWF0ZUFuZFNlbmRPZmZlcigpIHtcbiAgICBpZiAoIXRoaXMuX3BjKSB7XG4gICAgICBMb2dnZXIuZXJyb3IoJ1BlZXIgY29ubmVjdGlvbiBoYXZlIG5vdCBiZWVuIGNyZWF0ZWQuJyk7XG4gICAgICByZXR1cm47XG4gICAgfVxuICAgIHRoaXMuX2lzTmVnb3RpYXRpb25OZWVkZWQgPSBmYWxzZTtcbiAgICB0aGlzLl9wYy5jcmVhdGVPZmZlcigpLnRoZW4oKGRlc2MpID0+IHtcbiAgICAgIGRlc2Muc2RwID0gdGhpcy5fc2V0UnRwUmVjZWl2ZXJPcHRpb25zKGRlc2Muc2RwKTtcbiAgICAgIGlmICh0aGlzLl9wYy5zaWduYWxpbmdTdGF0ZSA9PT0gJ3N0YWJsZScgJiYgIXRoaXMuX3NldHRpbmdMb2NhbFNkcCAmJlxuICAgICAgICAgICF0aGlzLl9zZXR0aW5nUmVtb3RlU2RwKSB7XG4gICAgICAgIHRoaXMuX3NldHRpbmdMb2NhbFNkcCA9IHRydWU7XG4gICAgICAgIHJldHVybiB0aGlzLl9wYy5zZXRMb2NhbERlc2NyaXB0aW9uKGRlc2MpLnRoZW4oKCkgPT4ge1xuICAgICAgICAgIHRoaXMuX3NldHRpbmdMb2NhbFNkcCA9IGZhbHNlO1xuICAgICAgICAgIHJldHVybiB0aGlzLl9zZW5kU2RwKHRoaXMuX3BjLmxvY2FsRGVzY3JpcHRpb24pO1xuICAgICAgICB9KTtcbiAgICAgIH1cbiAgICB9KS5jYXRjaCgoZSkgPT4ge1xuICAgICAgTG9nZ2VyLmVycm9yKGUubWVzc2FnZSk7XG4gICAgICBjb25zdCBlcnJvciA9IG5ldyBFcnJvck1vZHVsZS5QMlBFcnJvcihFcnJvck1vZHVsZS5lcnJvcnMuUDJQX1dFQlJUQ19TRFAsXG4gICAgICAgICAgZS5tZXNzYWdlKTtcbiAgICAgIHRoaXMuX3N0b3AoZXJyb3IsIHRydWUpO1xuICAgIH0pO1xuICB9XG5cbiAgX2NyZWF0ZUFuZFNlbmRBbnN3ZXIoKSB7XG4gICAgdGhpcy5fZHJhaW5QZW5kaW5nU3RyZWFtcygpO1xuICAgIHRoaXMuX2lzTmVnb3RpYXRpb25OZWVkZWQgPSBmYWxzZTtcbiAgICB0aGlzLl9wYy5jcmVhdGVBbnN3ZXIoKS50aGVuKChkZXNjKSA9PiB7XG4gICAgICBkZXNjLnNkcCA9IHRoaXMuX3NldFJ0cFJlY2VpdmVyT3B0aW9ucyhkZXNjLnNkcCk7XG4gICAgICB0aGlzLl9sb2dDdXJyZW50QW5kUGVuZGluZ0xvY2FsRGVzY3JpcHRpb24oKTtcbiAgICAgIHRoaXMuX3NldHRpbmdMb2NhbFNkcCA9IHRydWU7XG4gICAgICByZXR1cm4gdGhpcy5fcGMuc2V0TG9jYWxEZXNjcmlwdGlvbihkZXNjKS50aGVuKCgpPT57XG4gICAgICAgIHRoaXMuX3NldHRpbmdMb2NhbFNkcCA9IGZhbHNlO1xuICAgICAgfSk7XG4gICAgfSkudGhlbigoKT0+e1xuICAgICAgcmV0dXJuIHRoaXMuX3NlbmRTZHAodGhpcy5fcGMubG9jYWxEZXNjcmlwdGlvbik7XG4gICAgfSkuY2F0Y2goKGUpID0+IHtcbiAgICAgIExvZ2dlci5lcnJvcihlLm1lc3NhZ2UgKyAnIFBsZWFzZSBjaGVjayB5b3VyIGNvZGVjIHNldHRpbmdzLicpO1xuICAgICAgY29uc3QgZXJyb3IgPSBuZXcgRXJyb3JNb2R1bGUuUDJQRXJyb3IoRXJyb3JNb2R1bGUuZXJyb3JzLlAyUF9XRUJSVENfU0RQLFxuICAgICAgICAgIGUubWVzc2FnZSk7XG4gICAgICB0aGlzLl9zdG9wKGVycm9yLCB0cnVlKTtcbiAgICB9KTtcbiAgfVxuXG4gIF9sb2dDdXJyZW50QW5kUGVuZGluZ0xvY2FsRGVzY3JpcHRpb24oKSB7XG4gICAgTG9nZ2VyLmluZm8oJ0N1cnJlbnQgZGVzY3JpcHRpb246ICcgKyB0aGlzLl9wYy5jdXJyZW50TG9jYWxEZXNjcmlwdGlvbik7XG4gICAgTG9nZ2VyLmluZm8oJ1BlbmRpbmcgZGVzY3JpcHRpb246ICcgKyB0aGlzLl9wYy5wZW5kaW5nTG9jYWxEZXNjcmlwdGlvbik7XG4gIH1cblxuICBfZ2V0QW5kRGVsZXRlVHJhY2tTb3VyY2VJbmZvKHRyYWNrcykge1xuICAgIGlmICh0cmFja3MubGVuZ3RoID4gMCkge1xuICAgICAgY29uc3QgdHJhY2tJZCA9IHRyYWNrc1swXS5pZDtcbiAgICAgIGlmICh0aGlzLl9yZW1vdGVUcmFja1NvdXJjZUluZm8uaGFzKHRyYWNrSWQpKSB7XG4gICAgICAgIGNvbnN0IHNvdXJjZUluZm8gPSB0aGlzLl9yZW1vdGVUcmFja1NvdXJjZUluZm8uZ2V0KHRyYWNrSWQpO1xuICAgICAgICB0aGlzLl9yZW1vdGVUcmFja1NvdXJjZUluZm8uZGVsZXRlKHRyYWNrSWQpO1xuICAgICAgICByZXR1cm4gc291cmNlSW5mbztcbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIExvZ2dlci53YXJuaW5nKCdDYW5ub3QgZmluZCBzb3VyY2UgaW5mbyBmb3IgJyArIHRyYWNrSWQpO1xuICAgICAgfVxuICAgIH1cbiAgfVxuXG4gIF91bnB1Ymxpc2goc3RyZWFtKSB7XG4gICAgaWYgKG5hdmlnYXRvci5tb3pHZXRVc2VyTWVkaWEgfHwgIXRoaXMuX3JlbW90ZVNpZGVTdXBwb3J0c1JlbW92ZVN0cmVhbSkge1xuICAgICAgLy8gQWN0dWFsbHkgdW5wdWJsaXNoIGlzIHN1cHBvcnRlZC4gSXQgaXMgYSBsaXR0bGUgYml0IGNvbXBsZXggc2luY2VcbiAgICAgIC8vIEZpcmVmb3ggaW1wbGVtZW50ZWQgV2ViUlRDIHNwZWMgd2hpbGUgQ2hyb21lIGltcGxlbWVudGVkIGFuIG9sZCBBUEkuXG4gICAgICBMb2dnZXIuZXJyb3IoXG4gICAgICAgICAgJ1N0b3BwaW5nIGEgcHVibGljYXRpb24gaXMgbm90IHN1cHBvcnRlZCBvbiBGaXJlZm94LiBQbGVhc2UgdXNlICcgK1xuICAgICAgICAgICdQMlBDbGllbnQuc3RvcCgpIHRvIHN0b3AgdGhlIGNvbm5lY3Rpb24gd2l0aCByZW1vdGUgZW5kcG9pbnQuJyxcbiAgICAgICk7XG4gICAgICByZXR1cm4gUHJvbWlzZS5yZWplY3QobmV3IEVycm9yTW9kdWxlLlAyUEVycm9yKFxuICAgICAgICAgIEVycm9yTW9kdWxlLmVycm9ycy5QMlBfQ0xJRU5UX1VOU1VQUE9SVEVEX01FVEhPRCkpO1xuICAgIH1cbiAgICBpZiAoIXRoaXMuX3B1Ymxpc2hlZFN0cmVhbXMuaGFzKHN0cmVhbSkpIHtcbiAgICAgIHJldHVybiBQcm9taXNlLnJlamVjdChuZXcgRXJyb3JNb2R1bGUuUDJQRXJyb3IoXG4gICAgICAgICAgRXJyb3JNb2R1bGUuZXJyb3JzLlAyUF9DTElFTlRfSUxMRUdBTF9BUkdVTUVOVCkpO1xuICAgIH1cbiAgICB0aGlzLl9wZW5kaW5nVW5wdWJsaXNoU3RyZWFtcy5wdXNoKHN0cmVhbSk7XG4gICAgcmV0dXJuIG5ldyBQcm9taXNlKChyZXNvbHZlLCByZWplY3QpID0+IHtcbiAgICAgIHRoaXMuX3VucHVibGlzaFByb21pc2VzLnNldChzdHJlYW0ubWVkaWFTdHJlYW0uaWQsIHtcbiAgICAgICAgcmVzb2x2ZTogcmVzb2x2ZSxcbiAgICAgICAgcmVqZWN0OiByZWplY3QsXG4gICAgICB9KTtcbiAgICAgIHRoaXMuX2RyYWluUGVuZGluZ1N0cmVhbXMoKTtcbiAgICB9KTtcbiAgfVxuXG4gIC8vIE1ha2Ugc3VyZSB8X3BjfCBpcyBhdmFpbGFibGUgYmVmb3JlIGNhbGxpbmcgdGhpcyBtZXRob2QuXG4gIF9jcmVhdGVEYXRhQ2hhbm5lbChsYWJlbCkge1xuICAgIGlmICh0aGlzLl9kYXRhQ2hhbm5lbHMuaGFzKGxhYmVsKSkge1xuICAgICAgTG9nZ2VyLndhcm5pbmcoJ0RhdGEgY2hhbm5lbCBsYWJlbGVkICcrIGxhYmVsKycgYWxyZWFkeSBleGlzdHMuJyk7XG4gICAgICByZXR1cm47XG4gICAgfVxuICAgIGlmICghdGhpcy5fcGMpIHtcbiAgICAgIExvZ2dlci5kZWJ1ZyhcbiAgICAgICAgICAnUGVlckNvbm5lY3Rpb24gaXMgbm90IGF2YWlsYWJsZSBiZWZvcmUgY3JlYXRpbmcgRGF0YUNoYW5uZWwuJyk7XG4gICAgICByZXR1cm47XG4gICAgfVxuICAgIExvZ2dlci5kZWJ1ZygnQ3JlYXRlIGRhdGEgY2hhbm5lbC4nKTtcbiAgICBjb25zdCBkYyA9IHRoaXMuX3BjLmNyZWF0ZURhdGFDaGFubmVsKGxhYmVsKTtcbiAgICB0aGlzLl9iaW5kRXZlbnRzVG9EYXRhQ2hhbm5lbChkYyk7XG4gICAgdGhpcy5fZGF0YUNoYW5uZWxzLnNldChEYXRhQ2hhbm5lbExhYmVsLk1FU1NBR0UsIGRjKTtcbiAgfVxuXG4gIF9iaW5kRXZlbnRzVG9EYXRhQ2hhbm5lbChkYykge1xuICAgIGRjLm9ubWVzc2FnZSA9IChldmVudCkgPT4ge1xuICAgICAgdGhpcy5fb25EYXRhQ2hhbm5lbE1lc3NhZ2UuYXBwbHkodGhpcywgW2V2ZW50XSk7XG4gICAgfTtcbiAgICBkYy5vbm9wZW4gPSAoZXZlbnQpID0+IHtcbiAgICAgIHRoaXMuX29uRGF0YUNoYW5uZWxPcGVuLmFwcGx5KHRoaXMsIFtldmVudF0pO1xuICAgIH07XG4gICAgZGMub25jbG9zZSA9IChldmVudCkgPT4ge1xuICAgICAgdGhpcy5fb25EYXRhQ2hhbm5lbENsb3NlLmFwcGx5KHRoaXMsIFtldmVudF0pO1xuICAgIH07XG4gICAgZGMub25lcnJvciA9IChldmVudCkgPT4ge1xuICAgICAgTG9nZ2VyLmRlYnVnKCdEYXRhIENoYW5uZWwgRXJyb3I6ICcgKyBldmVudCk7XG4gICAgfTtcbiAgfVxuXG4gIC8vIFJldHVybnMgYWxsIE1lZGlhU3RyZWFtcyBpdCBiZWxvbmdzIHRvLlxuICBfZ2V0U3RyZWFtQnlUcmFjayhtZWRpYVN0cmVhbVRyYWNrKSB7XG4gICAgY29uc3Qgc3RyZWFtcyA9IFtdO1xuICAgIGZvciAoY29uc3QgWy8qIGlkICovLCBpbmZvXSBvZiB0aGlzLl9yZW1vdGVTdHJlYW1JbmZvKSB7XG4gICAgICBpZiAoIWluZm8uc3RyZWFtIHx8ICFpbmZvLnN0cmVhbS5tZWRpYVN0cmVhbSkge1xuICAgICAgICBjb250aW51ZTtcbiAgICAgIH1cbiAgICAgIGZvciAoY29uc3QgdHJhY2sgb2YgaW5mby5zdHJlYW0ubWVkaWFTdHJlYW0uZ2V0VHJhY2tzKCkpIHtcbiAgICAgICAgaWYgKG1lZGlhU3RyZWFtVHJhY2sgPT09IHRyYWNrKSB7XG4gICAgICAgICAgc3RyZWFtcy5wdXNoKGluZm8uc3RyZWFtLm1lZGlhU3RyZWFtKTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gc3RyZWFtcztcbiAgfVxuXG4gIF9hcmVBbGxUcmFja3NFbmRlZChtZWRpYVN0cmVhbSkge1xuICAgIGZvciAoY29uc3QgdHJhY2sgb2YgbWVkaWFTdHJlYW0uZ2V0VHJhY2tzKCkpIHtcbiAgICAgIGlmICh0cmFjay5yZWFkeVN0YXRlID09PSAnbGl2ZScpIHtcbiAgICAgICAgcmV0dXJuIGZhbHNlO1xuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gdHJ1ZTtcbiAgfVxuXG4gIF9zdG9wKGVycm9yLCBub3RpZnlSZW1vdGUpIHtcbiAgICBsZXQgcHJvbWlzZUVycm9yID0gZXJyb3I7XG4gICAgaWYgKCFwcm9taXNlRXJyb3IpIHtcbiAgICAgIHByb21pc2VFcnJvciA9IG5ldyBFcnJvck1vZHVsZS5QMlBFcnJvcihcbiAgICAgICAgICBFcnJvck1vZHVsZS5lcnJvcnMuUDJQX0NMSUVOVF9VTktOT1dOKTtcbiAgICB9XG4gICAgZm9yIChjb25zdCBbLyogbGFiZWwgKi8sIGRjXSBvZiB0aGlzLl9kYXRhQ2hhbm5lbHMpIHtcbiAgICAgIGRjLmNsb3NlKCk7XG4gICAgfVxuICAgIHRoaXMuX2RhdGFDaGFubmVscy5jbGVhcigpO1xuICAgIGlmICh0aGlzLl9wYyAmJiB0aGlzLl9wYy5pY2VDb25uZWN0aW9uU3RhdGUgIT09ICdjbG9zZWQnKSB7XG4gICAgICB0aGlzLl9wYy5jbG9zZSgpO1xuICAgIH1cbiAgICBmb3IgKGNvbnN0IFsvKiBpZCAqLywgcHJvbWlzZV0gb2YgdGhpcy5fcHVibGlzaFByb21pc2VzKSB7XG4gICAgICBwcm9taXNlLnJlamVjdChwcm9taXNlRXJyb3IpO1xuICAgIH1cbiAgICB0aGlzLl9wdWJsaXNoUHJvbWlzZXMuY2xlYXIoKTtcbiAgICBmb3IgKGNvbnN0IFsvKiBpZCAqLywgcHJvbWlzZV0gb2YgdGhpcy5fdW5wdWJsaXNoUHJvbWlzZXMpIHtcbiAgICAgIHByb21pc2UucmVqZWN0KHByb21pc2VFcnJvcik7XG4gICAgfVxuICAgIHRoaXMuX3VucHVibGlzaFByb21pc2VzLmNsZWFyKCk7XG4gICAgZm9yIChjb25zdCBbLyogaWQgKi8sIHByb21pc2VdIG9mIHRoaXMuX3NlbmREYXRhUHJvbWlzZXMpIHtcbiAgICAgIHByb21pc2UucmVqZWN0KHByb21pc2VFcnJvcik7XG4gICAgfVxuICAgIHRoaXMuX3NlbmREYXRhUHJvbWlzZXMuY2xlYXIoKTtcbiAgICAvLyBGaXJlIGVuZGVkIGV2ZW50IGlmIHB1YmxpY2F0aW9uIG9yIHJlbW90ZSBzdHJlYW0gZXhpc3RzLlxuICAgIHRoaXMuX3B1Ymxpc2hlZFN0cmVhbXMuZm9yRWFjaCgocHVibGljYXRpb24pID0+IHtcbiAgICAgIHB1YmxpY2F0aW9uLmRpc3BhdGNoRXZlbnQobmV3IE93dEV2ZW50KCdlbmRlZCcpKTtcbiAgICB9KTtcbiAgICB0aGlzLl9wdWJsaXNoZWRTdHJlYW1zLmNsZWFyKCk7XG4gICAgdGhpcy5fcmVtb3RlU3RyZWFtcy5mb3JFYWNoKChzdHJlYW0pID0+IHtcbiAgICAgIHN0cmVhbS5kaXNwYXRjaEV2ZW50KG5ldyBPd3RFdmVudCgnZW5kZWQnKSk7XG4gICAgfSk7XG4gICAgdGhpcy5fcmVtb3RlU3RyZWFtcyA9IFtdO1xuICAgIGlmICghdGhpcy5fZGlzcG9zZWQpIHtcbiAgICAgIGlmIChub3RpZnlSZW1vdGUpIHtcbiAgICAgICAgbGV0IHNlbmRFcnJvcjtcbiAgICAgICAgaWYgKGVycm9yKSB7XG4gICAgICAgICAgc2VuZEVycm9yID0gSlNPTi5wYXJzZShKU09OLnN0cmluZ2lmeShlcnJvcikpO1xuICAgICAgICAgIC8vIEF2b2lkIHRvIGxlYWsgZGV0YWlsZWQgZXJyb3IgdG8gcmVtb3RlIHNpZGUuXG4gICAgICAgICAgc2VuZEVycm9yLm1lc3NhZ2UgPSAnRXJyb3IgaGFwcGVuZWQgYXQgcmVtb3RlIHNpZGUuJztcbiAgICAgICAgfVxuICAgICAgICB0aGlzLl9zZW5kU2lnbmFsaW5nTWVzc2FnZShTaWduYWxpbmdUeXBlLkNMT1NFRCwgc2VuZEVycm9yKS5jYXRjaChcbiAgICAgICAgICAgIChlcnIpID0+IHtcbiAgICAgICAgICAgICAgTG9nZ2VyLmRlYnVnKCdGYWlsZWQgdG8gc2VuZCBjbG9zZS4nICsgZXJyLm1lc3NhZ2UpO1xuICAgICAgICAgICAgfSk7XG4gICAgICB9XG4gICAgICB0aGlzLmRpc3BhdGNoRXZlbnQobmV3IEV2ZW50KCdlbmRlZCcpKTtcbiAgICB9XG4gIH1cblxuICBfc2V0U3RyZWFtVG9SZW1vdGVTdHJlYW1JbmZvKG1lZGlhU3RyZWFtKSB7XG4gICAgY29uc3QgaW5mbyA9IHRoaXMuX3JlbW90ZVN0cmVhbUluZm8uZ2V0KG1lZGlhU3RyZWFtLmlkKTtcbiAgICBjb25zdCBhdHRyaWJ1dGVzID0gaW5mby5hdHRyaWJ1dGVzO1xuICAgIGNvbnN0IHNvdXJjZUluZm8gPSBuZXcgU3RyZWFtTW9kdWxlLlN0cmVhbVNvdXJjZUluZm8odGhpcy5fcmVtb3RlU3RyZWFtSW5mb1xuICAgICAgICAuZ2V0KG1lZGlhU3RyZWFtLmlkKS5zb3VyY2UuYXVkaW8sIHRoaXMuX3JlbW90ZVN0cmVhbUluZm8uZ2V0KFxuICAgICAgICBtZWRpYVN0cmVhbS5pZClcbiAgICAgICAgLnNvdXJjZS52aWRlbyk7XG4gICAgaW5mby5zdHJlYW0gPSBuZXcgU3RyZWFtTW9kdWxlLlJlbW90ZVN0cmVhbShcbiAgICAgICAgdW5kZWZpbmVkLCB0aGlzLl9yZW1vdGVJZCwgbWVkaWFTdHJlYW0sXG4gICAgICAgIHNvdXJjZUluZm8sIGF0dHJpYnV0ZXMpO1xuICAgIGluZm8ubWVkaWFTdHJlYW0gPSBtZWRpYVN0cmVhbTtcbiAgICBjb25zdCBzdHJlYW0gPSBpbmZvLnN0cmVhbTtcbiAgICBpZiAoc3RyZWFtKSB7XG4gICAgICB0aGlzLl9yZW1vdGVTdHJlYW1zLnB1c2goc3RyZWFtKTtcbiAgICB9IGVsc2Uge1xuICAgICAgTG9nZ2VyLndhcm5pbmcoJ0ZhaWxlZCB0byBjcmVhdGUgUmVtb3RlU3RyZWFtLicpO1xuICAgIH1cbiAgfVxuXG4gIF9jaGVja0ljZUNvbm5lY3Rpb25TdGF0ZUFuZEZpcmVFdmVudCgpIHtcbiAgICBpZiAodGhpcy5fcGMuaWNlQ29ubmVjdGlvblN0YXRlID09PSAnY29ubmVjdGVkJyB8fFxuICAgICAgICB0aGlzLl9wYy5pY2VDb25uZWN0aW9uU3RhdGUgPT09ICdjb21wbGV0ZWQnKSB7XG4gICAgICBmb3IgKGNvbnN0IFsvKiBpZCAqLywgaW5mb10gb2YgdGhpcy5fcmVtb3RlU3RyZWFtSW5mbykge1xuICAgICAgICBpZiAoaW5mby5tZWRpYVN0cmVhbSkge1xuICAgICAgICAgIGNvbnN0IHN0cmVhbUV2ZW50ID0gbmV3IFN0cmVhbU1vZHVsZS5TdHJlYW1FdmVudCgnc3RyZWFtYWRkZWQnLCB7XG4gICAgICAgICAgICBzdHJlYW06IGluZm8uc3RyZWFtLFxuICAgICAgICAgIH0pO1xuICAgICAgICAgIGZvciAoY29uc3QgdHJhY2sgb2YgaW5mby5tZWRpYVN0cmVhbS5nZXRUcmFja3MoKSkge1xuICAgICAgICAgICAgdHJhY2suYWRkRXZlbnRMaXN0ZW5lcignZW5kZWQnLCAoZXZlbnQpID0+IHtcbiAgICAgICAgICAgICAgY29uc3QgbWVkaWFTdHJlYW1zID0gdGhpcy5fZ2V0U3RyZWFtQnlUcmFjayhldmVudC50YXJnZXQpO1xuICAgICAgICAgICAgICBmb3IgKGNvbnN0IG1lZGlhU3RyZWFtIG9mIG1lZGlhU3RyZWFtcykge1xuICAgICAgICAgICAgICAgIGlmICh0aGlzLl9hcmVBbGxUcmFja3NFbmRlZChtZWRpYVN0cmVhbSkpIHtcbiAgICAgICAgICAgICAgICAgIHRoaXMuX29uUmVtb3RlU3RyZWFtUmVtb3ZlZCh7c3RyZWFtOiBtZWRpYVN0cmVhbX0pO1xuICAgICAgICAgICAgICAgIH1cbiAgICAgICAgICAgICAgfVxuICAgICAgICAgICAgfSk7XG4gICAgICAgICAgfVxuICAgICAgICAgIHRoaXMuX3NlbmRTaWduYWxpbmdNZXNzYWdlKFNpZ25hbGluZ1R5cGUuVFJBQ0tTX0FEREVELCBpbmZvLnRyYWNrSWRzKTtcbiAgICAgICAgICB0aGlzLl9yZW1vdGVTdHJlYW1JbmZvLmdldChpbmZvLm1lZGlhU3RyZWFtLmlkKS5tZWRpYVN0cmVhbSA9IG51bGw7XG4gICAgICAgICAgdGhpcy5kaXNwYXRjaEV2ZW50KHN0cmVhbUV2ZW50KTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgIH1cbiAgfVxufVxuXG5leHBvcnQgZGVmYXVsdCBQMlBQZWVyQ29ubmVjdGlvbkNoYW5uZWw7XG4iXX0=
