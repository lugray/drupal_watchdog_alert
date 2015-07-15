<?php

/**
 * @file Demonstrates examples of how to use the hooks defined in this module.
 */

/**
 * Provide a list of known watchdog message types
 *
 * This allows Watchdog Alert to prepopulate its types table so that alert
 * profiles may be created which for message types that heven't occured since
 * Watchdog Alert was installed.
 *
 * There are two main forseen use cases to implement this hook:
 * 1. Your module implements hook_watchdog() and stores the types somewhere,
 *    and so you can retrieve them and return them in this hook.
 * 2. Your module has calls to watchdog() with known type(s) (usually your
 *    module's name) and can return a list of those types.
 *
 * @return string[]
 *   A list of watchodg types known to the module.  Array keys are ignored.
 */
function hook_watchdog_alert_types() {
  return array(
    'watchdog_alert',
  );
}