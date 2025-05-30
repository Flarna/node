#include <node_api.h>
#include <assert.h>
#include <string.h>
#include <uv.h>
#include "../../js-native-api/common.h"

typedef struct {
  napi_env env;
  char* stack;
} ThreadData;
static ThreadData thread_data[2];
static uint32_t idx;
uv_thread_t uv_thread;

static ThreadData* getThreadData(napi_env env) {
  for (uint32_t i = 0; i < idx; ++i) {
    ThreadData *td = thread_data + i;
    if (td->env == env) {
      return td;
    }
  }
  return NULL;
}

static void InterruptCallback(napi_env env, void* data) {
  printf("InterruptCallback 1, %p, %p\n", env, data);
  ThreadData* td = getThreadData(env);
  NODE_API_ASSERT_RETURN_VOID(env, td != NULL, "ThreadData not found");
  NODE_API_ASSERT_RETURN_VOID(env, td == data, "ThreadData != data");
  printf("InterruptCallback 2, %p, %p\n", td, td->stack);

  // create an error object
  napi_value message;
  napi_value error;
  NODE_API_CALL_RETURN_VOID(
      env, napi_create_string_utf8(env, "dummy", NAPI_AUTO_LENGTH, &message));
  NODE_API_CALL_RETURN_VOID(env, napi_create_error(env, NULL, message, &error));
  
  // extract stack
  napi_value stack;
  NODE_API_CALL_RETURN_VOID(
      env, napi_get_named_property(env, error, "stack", &stack));
  size_t str_len;
  NODE_API_CALL_RETURN_VOID(
      env, napi_get_value_string_utf8(env, stack, NULL, 0, &str_len));
  td->stack = malloc(str_len + 1);
  NODE_API_ASSERT_RETURN_VOID(env, td->stack != NULL, "Allocation of stack string failed");
  NODE_API_CALL_RETURN_VOID(
      env,
      napi_get_value_string_utf8(env, stack, td->stack, str_len + 1, &str_len));
  printf("InterruptCallback 3, %s\n", td->stack);

  // signal js via global.irq_done to exit loop 
  napi_value global;
  NODE_API_CALL_RETURN_VOID(env, napi_get_global(env, &global));
  napi_value t;
  NODE_API_CALL_RETURN_VOID(env, napi_get_boolean(env, true, &t));
  NODE_API_CALL_RETURN_VOID(env, napi_set_named_property(env, global, "irq_done", t));
  printf("InterruptCallback 4\n");
}

static napi_value GetStacks(napi_env env, napi_callback_info info) {
  printf("GetStacks 1, %p\n", env);
  napi_value stacks;
  NODE_API_CALL(env, napi_create_array_with_length(env, idx, &stacks));
  for (uint32_t i = 0; i < idx; ++i) {
    ThreadData *td = thread_data + i;
    printf("GetStacks 2.%u, %p, %p\n", i, td, td->stack);
    if (td->stack != NULL) {
      printf("GetStacks 3.%u, %s\n", i, td->stack);
      napi_value stack;
      NODE_API_CALL(
          env, napi_create_string_utf8(env, td->stack, NAPI_AUTO_LENGTH, &stack));
      free(td->stack);
      td->stack = NULL;
      NODE_API_CALL(env, napi_set_element(env, stacks, i, stack));
    }
  }
  NODE_API_ASSERT(env, (uv_thread_join(&uv_thread) == 0), "Thread join");
  printf("GetStacks 4\n");
  return stacks;
}

static void thread_fn(void* data) {
  for (uint32_t i = 0; i < idx; ++i) {
    ThreadData *td = thread_data + i;
    NODE_API_CALL_RETURN_VOID(
        td->env,
        node_api_request_interrupt(td->env, InterruptCallback, td));
  }
}

static napi_value TriggerIrqs(napi_env env, napi_callback_info info) {
  // trigger interrupts from foreign thread
  NODE_API_ASSERT(env,
      (uv_thread_create(&uv_thread, thread_fn, NULL) == 0),
      "Thread creation");
  return NULL;
}

static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("GetStacks", GetStacks),
    DECLARE_NODE_API_PROPERTY("TriggerIrqs", TriggerIrqs),
  };

  NODE_API_CALL(
      env,
      napi_define_properties(env,
                             exports,
                             sizeof(descriptors) / sizeof(*descriptors),
                             descriptors));
 
  if (idx < 2u) {
    printf("Init %p\n", env);
    thread_data[idx++].env = env;
  }
  
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
