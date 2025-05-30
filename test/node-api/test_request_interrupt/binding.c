#include <node_api.h>
#include <assert.h>
#include <string.h>
#include "../../js-native-api/common.h"

typedef struct {
  napi_env env;
  char* stack;
} ThreadData;
static ThreadData thread_data[2];
static int idx;

static void InterruptCallback(napi_env env, void* data) {
  napi_value message;
  napi_value error;
  NODE_API_CALL(
      env, napi_create_string_utf8(env, "dummy", NAPI_AUTO_LENGTH, &message));
  NODE_API_CALL(env, napi_create_error(env, NULL, message, &error));
  
  napi_value stack;
  NODE_API_CALL(env, napi_get_named_property(env, error, "stack", &stack));
  size_t str_len;
  NODE_API_CALL(env, napi_get_value_string_utf8(env, stack, NULL, 0, &str_len));
  irq_stack = malloc(str_len + 1);
  NODE_API_CALL(
      env,
      napi_get_value_string_utf8(env, stack, irq_stack, str_len + 1, &str_len));

  napi_value global;
  NODE_API_CALL(env, napi_get_global(env, &global));
  napi_value t;
  NODE_API_CALL(env, napi_get_boolean(env, true, &t));
  NODE_API_CALL(env, napi_set_named_property(env, global, "irq_done", t));
}

static napi_value GetStack(napi_env env, napi_callback_info info) {
  napi_value stack;
  if (irq_stack != NULL) {
    NODE_API_CALL(
        env, napi_create_string_utf8(env, irq_stack, NAPI_AUTO_LENGTH, &stack));
    free(irq_stack);
    irq_stack = NULL;
  }
  return stack;
}

static napi_value TriggerIrq(napi_env env, napi_callback_info info) {
  NODE_API_CALL(
      env,
      node_api_request_interrupt(env, InterruptCallback, InterruptCallback));
  return NULL;
}

static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("GetStack", GetStack),
    DECLARE_NODE_API_PROPERTY("TriggerIrq", TriggerIrq),
  };

  NODE_API_CALL(
      env,
      napi_define_properties(env,
                             exports,
                             sizeof(descriptors) / sizeof(*descriptors),
                             descriptors));
 
  if (i < 2) {
    thread_data[i++].env = env;
  }
  
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
