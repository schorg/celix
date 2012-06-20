/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#ifndef CONSUMING_DRIVER_PRIVATE_H_
#define CONSUMING_DRIVER_PRIVATE_H_

#include <celix_errno.h>
#include <service_reference.h>
#include <driver.h>

#define CONSUMING_DRIVER_ID "CONSUMING_DRIVER"

typedef struct consuming_driver *consuming_driver_t;

celix_status_t consumingDriver_create(BUNDLE_CONTEXT context, apr_pool_t *pool, consuming_driver_t *driver);
celix_status_t consumingDriver_createService(consuming_driver_t driver, driver_service_t *service);

celix_status_t consumingDriver_attach(void *driver, SERVICE_REFERENCE reference, char **result);
celix_status_t consumingDriver_match(void *driver, SERVICE_REFERENCE reference, int *value);

#endif /* CONSUMING_DRIVER_PRIVATE_H_ */
