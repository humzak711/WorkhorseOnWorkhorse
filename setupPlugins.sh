#!/usr/bin/env bash

set -e

cp -r hostPlugins/. WorkhorseRT/plugins/
cp -r guestPlugins/. guest/plugins/