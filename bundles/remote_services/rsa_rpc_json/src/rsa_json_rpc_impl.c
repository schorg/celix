/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <rsa_json_rpc_impl.h>
#include <rsa_json_rpc_constants.h>
#include <rsa_json_rpc_endpoint_impl.h>
#include <rsa_json_rpc_proxy_impl.h>
#include <rsa_request_sender_tracker.h>
#include <rsa_request_handler_service.h>
#include <remote_interceptors_handler.h>
#include <endpoint_description.h>
#include <celix_long_hash_map.h>
#include <celix_log_helper.h>
#include <dyn_interface.h>
#include <celix_api.h>
#include <limits.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

struct rsa_json_rpc {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    celix_thread_mutex_t mutex; //It protects svcProxyFactories and svcEndpoints
    celix_long_hash_map_t *svcProxyFactories;// Key: proxy factory service id, Value: rsa_json_rpc_proxy_fac_entry
    celix_long_hash_map_t *svcEndpoints;// Key:request handler service id, Value: rsa_json_rpc_endpoint_entry
    remote_interceptors_handler_t *interceptorsHandler;
    rsa_request_sender_tracker_t *reqSenderTracker;
    FILE *callsLogFile;
};

struct rsa_json_rpc_endpoint_entry {
    rsa_json_rpc_endpoint_t *endpoint;
    rsa_request_handler_service_t reqHandlerSvc;
};

celix_status_t rsaJsonRpc_create(celix_bundle_context_t* ctx, celix_log_helper_t *logHelper,
        rsa_json_rpc_t **jsonRpcOut) {
    celix_status_t status = CELIX_SUCCESS;
    if (ctx == NULL || logHelper == NULL || jsonRpcOut == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    rsa_json_rpc_t *rpc = calloc(1, sizeof(rsa_json_rpc_t));
    assert(rpc != NULL);
    rpc->ctx = ctx;
    rpc->logHelper = logHelper;
    status = celixThreadMutex_create(&rpc->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Error creating endpoint mutex. %d.", status);
        goto mutex_err;
    }
    rpc->svcProxyFactories = celix_longHashMap_create();
    assert(rpc->svcProxyFactories != NULL);
    rpc->svcEndpoints = celix_longHashMap_create();
    assert(rpc->svcEndpoints != NULL);

    status = remoteInterceptorsHandler_create(ctx, &rpc->interceptorsHandler);
    if (status != CELIX_SUCCESS) {
        goto interceptors_err;
    }

    status = rsaRequestSenderTracker_create(ctx, "rsa_json_rpc_rst", &rpc->reqSenderTracker);
    if (status != CELIX_SUCCESS) {
        goto rst_err;
    }

    bool logCalls = celix_bundleContext_getPropertyAsBool(ctx, RSA_JSON_RPC_LOG_CALLS_KEY, RSA_JSON_RPC_LOG_CALLS_DEFAULT);
    if (logCalls) {
        const char *f = celix_bundleContext_getProperty(ctx, RSA_JSON_RPC_LOG_CALLS_FILE_KEY, RSA_JSON_RPC_LOG_CALLS_FILE_DEFAULT);
        if (strncmp(f, "stdout", strlen("stdout")) == 0) {
            rpc->callsLogFile = stdout;
        } else {
            rpc->callsLogFile = fopen(f, "w");
            if (rpc->callsLogFile == NULL) {
                celix_logHelper_warning(logHelper, "Error opening file '%s' for logging calls. %d", f, errno);
            }
        }
    }
    *jsonRpcOut = rpc;
    return CELIX_SUCCESS;
rst_err:
    remoteInterceptorsHandler_destroy(rpc->interceptorsHandler);
interceptors_err:
    celix_longHashMap_destroy(rpc->svcEndpoints);
    celix_longHashMap_destroy(rpc->svcProxyFactories);
    (void)celixThreadMutex_destroy(&rpc->mutex);
mutex_err:
    free(rpc);
    return status;
}

void rsaJsonRpc_destroy(rsa_json_rpc_t *jsonRpc) {
    if (jsonRpc != NULL) {
        if (jsonRpc->callsLogFile != NULL && jsonRpc->callsLogFile != stdout) {
            fclose(jsonRpc->callsLogFile);
        }
        rsaRequestSenderTracker_destroy(jsonRpc->reqSenderTracker);
        remoteInterceptorsHandler_destroy(jsonRpc->interceptorsHandler);
        assert(celix_longHashMap_size(jsonRpc->svcEndpoints) == 0);
        celix_longHashMap_destroy(jsonRpc->svcEndpoints);
        assert(celix_longHashMap_size(jsonRpc->svcProxyFactories) == 0);
        celix_longHashMap_destroy(jsonRpc->svcProxyFactories);
        (void)celixThreadMutex_destroy(&jsonRpc->mutex);
        free(jsonRpc);
    }
    return;
}

celix_status_t rsaJsonRpc_installProxy(void *handle, const endpoint_description_t *endpointDesc,
        long requestSenderSvcId, long *proxySvcId) {
    celix_status_t status= CELIX_SUCCESS;

    if (handle == NULL || endpointDescription_isInvalid(endpointDesc)
            || requestSenderSvcId < 0  || proxySvcId == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)handle;

    rsa_json_rpc_proxy_factory_t *proxyFactory = NULL;
    status = rsaJsonRpcProxy_factoryCreate(jsonRpc->ctx, jsonRpc->logHelper,
            jsonRpc->callsLogFile, jsonRpc->interceptorsHandler, endpointDesc,
            jsonRpc->reqSenderTracker, requestSenderSvcId, &proxyFactory);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(jsonRpc->logHelper, "Error creating proxy factory for %s.", endpointDesc->service);
        goto err_creating_proxy_fac;
    }
    long factorySvcId = rsaJsonRpcProxy_factorySvcId(proxyFactory);

    celixThreadMutex_lock(&jsonRpc->mutex);
    celix_longHashMap_put(jsonRpc->svcProxyFactories, factorySvcId, proxyFactory);
    celixThreadMutex_unlock(&jsonRpc->mutex);
    *proxySvcId = factorySvcId;

    return CELIX_SUCCESS;

err_creating_proxy_fac:
    return status;
}

void rsaJsonRpc_uninstallProxy(void *handle, long proxySvcId) {
    if (handle == NULL  || proxySvcId < 0) {
        return;
    }
    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)handle;
    celixThreadMutex_lock(&jsonRpc->mutex);
    rsa_json_rpc_proxy_factory_t *proxyFactory =
            celix_longHashMap_get(jsonRpc->svcProxyFactories, proxySvcId);
    if (proxyFactory != NULL) {
        (void)celix_longHashMap_remove(jsonRpc->svcProxyFactories, proxySvcId);
        rsaJsonRpcProxy_factoryDestroy(proxyFactory);
    }
    celixThreadMutex_unlock(&jsonRpc->mutex);
    return;
}

celix_status_t rsaJsonRpc_installEndpoint(void *handle, const endpoint_description_t *endpointDesc,
        long *requestHandlerSvcId) {
    celix_status_t status= CELIX_SUCCESS;
    if (handle == NULL || endpointDescription_isInvalid(endpointDesc) || requestHandlerSvcId == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)handle;

    struct rsa_json_rpc_endpoint_entry *endpointEntry = calloc(1, sizeof(*endpointEntry));
    assert(endpointEntry != NULL);

    status = rsaJsonRpcEndpoint_create(jsonRpc->ctx, jsonRpc->logHelper, jsonRpc->callsLogFile,
            jsonRpc->interceptorsHandler, endpointDesc, &endpointEntry->endpoint);
    if (status != CELIX_SUCCESS) {
        goto endpoint_err;
    }
    endpointEntry->reqHandlerSvc.handle = endpointEntry->endpoint;
    endpointEntry->reqHandlerSvc.handleRequest = rsaJsonRpcEndpoint_handleRequest;
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.serviceName = RSA_REQUEST_HANDLER_SERVICE_NAME;
    opts.serviceVersion = RSA_REQUEST_HANDLER_SERVICE_VERSION;
    opts.svc = &endpointEntry->reqHandlerSvc;
    long reqHandlerSvcId = celix_bundleContext_registerServiceWithOptionsAsync(jsonRpc->ctx, &opts);
    if (reqHandlerSvcId< 0) {
        celix_logHelper_error(jsonRpc->logHelper, "Error Registering endpoint service.");
        goto endpoint_svc_err;
    }

    celixThreadMutex_lock(&jsonRpc->mutex);
    celix_longHashMap_put(jsonRpc->svcEndpoints, reqHandlerSvcId, endpointEntry);
    celixThreadMutex_unlock(&jsonRpc->mutex);
    *requestHandlerSvcId = reqHandlerSvcId;

    return CELIX_SUCCESS;

endpoint_svc_err:
    rsaJsonRpcEndpoint_destroy(endpointEntry->endpoint);
endpoint_err:
    free(endpointEntry);
    return status;
}

static void rsaJsonRpc_unregistereEndpointSvcDone(void *data) {
    assert(data != NULL);
    struct rsa_json_rpc_endpoint_entry *endpointEntry = (struct rsa_json_rpc_endpoint_entry *)data;
    rsaJsonRpcEndpoint_destroy(endpointEntry->endpoint);
    free(endpointEntry);
    return;
}

void rsaJsonRpc_uninstallEndpoint(void *handle, long requestHandlerSvcId) {
    if (handle == NULL  || requestHandlerSvcId < 0) {
        return;
    }
    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)handle;
    celixThreadMutex_lock(&jsonRpc->mutex);

    struct rsa_json_rpc_endpoint_entry *endpointEntry =
            celix_longHashMap_get(jsonRpc->svcEndpoints, requestHandlerSvcId);
    if (endpointEntry != NULL) {
        celix_longHashMap_remove(jsonRpc->svcEndpoints, requestHandlerSvcId);
        celix_bundleContext_unregisterServiceAsync(jsonRpc->ctx, requestHandlerSvcId,
                endpointEntry, rsaJsonRpc_unregistereEndpointSvcDone);
    }
    celixThreadMutex_unlock(&jsonRpc->mutex);
    return;
}

