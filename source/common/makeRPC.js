// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

exports.makeRPC = function (rpcClient, remote_node, remote_function, parameters_list, on_ok, on_error) {
    rpcClient.remoteCall(
        remote_node,
        remote_function,
        parameters_list,
        {callback: function (result, error_reason) {
            if (result === 'error') {
                typeof on_error === 'function' && on_error(error_reason);
            } else if (result === 'timeout') {
                typeof on_error === 'function' && on_error('Timeout to make rpc to ' + remote_node + '.' + remote_function);
            } else {
                typeof on_ok === 'function' && on_ok(result);
            }
        }}
    );
};
