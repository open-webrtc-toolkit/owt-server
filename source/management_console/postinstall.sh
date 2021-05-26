#!/usr/bin/env bash

# vendor directory
mkdir -p public/vendor/css
mkdir -p public/vendor/js
mkdir -p public/vendor/fonts
# jquery
cp node_modules/jquery/dist/jquery.min.js public/vendor/js/
# bootstrap
cp node_modules/bootstrap/dist/css/bootstrap.min.css public/vendor/css/
cp node_modules/bootstrap/dist/js/bootstrap.min.js public/vendor/js/
cp -r node_modules/bootstrap/dist/fonts/* public/vendor/fonts
# lodash
cp node_modules/lodash/lodash.min.js public/vendor/js/
# pnotify js & css
cp node_modules/pnotify/dist/pnotify.css public/vendor/css/
cp node_modules/pnotify/dist/pnotify.buttons.css public/vendor/css/
cp node_modules/pnotify/dist/pnotify.js public/vendor/js/
cp node_modules/pnotify/dist/pnotify.confirm.js public/vendor/js/
cp node_modules/pnotify/dist/pnotify.buttons.js public/vendor/js/
rm -rf node_modules/pnotify/build-tools/
# react
cp node_modules/react/umd/react.production.min.js public/vendor/js/
cp node_modules/react-dom/umd/react-dom.production.min.js public/vendor/js/
# react-table
cp node_modules/react-table/react-table.css public/vendor/css/
cp node_modules/react-table/react-table.min.js public/vendor/js/