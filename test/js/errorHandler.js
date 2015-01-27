var gOldOnError = window.onerror;

window.onerror = function errorHandler(errMsg, url, lineNumber, colNumber, error) {
  console.log('****************** catch error info *********');
  console.log(errMsg);
  console.log('url:' + url);
  console.log('line number:' + lineNumber);
  console.log('colNumber:' + colNumber);
  console.log('****************** catch error info ********');
  if (error instanceof TypeError) {
    // prevent firing the default error handler so that can continue testing.
    return true;
  } else {
    return false;
  }
}
