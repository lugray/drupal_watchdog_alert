<?php

/**
 * @file
 * Provide callback functions for the administrative interface for the Database
 * Logging Alert module
 */
/**
 * This module stores sets of severities as a bit string represented as an
 * integer where each bit represents inclusion of that severity level.  This
 * constant, then, is useful for testing/setting inclusion of all severities.
 */
define('WATCHDOG_ALERT_ALL_SEVERITIES', pow(2, max(array_keys(watchdog_severity_levels())) + 1) - 1);

/**
 * Page callback: Log Message Alert Profiles
 *
 * Display a table of existing alert profiles, giving the option to edit/delete
 * them, as well as a button to create a new one.
 *
 * @see watchdog_alert_menu()
 */
function _watchdog_alert_profiles() {
  ctools_include('dropbutton.theme');
  $header = array(
    array('data' => t('Type'), 'field' => 't.type', 'sort' => 'asc'),
    array('data' => t('Severity'), 'field' => 'p.severity'),
    array('data' => t('Role'), 'field' => 'p.rid'),
    array('data' => t('Show Repeated Alerts'), 'field' => 'p.repeat_alert'),
    array('data' => t('Dismiss Automatically'), 'field' => 'p.auto_dismiss'),
    array('data' => t('Dismiss for All'), 'field' => 'p.dismiss_for_all'),
    t('Operations'),
  );

  $results = db_select('watchdog_alert_profile', 'p')
      ->join('watchdog_alert_type', 't' . 't.tid = p.tid')
      ->extend('PagerDefault')
      ->extend('TableSort')
      ->fields('p')
      ->fields('t', array('type'))
      ->limit(50)
      ->orderByHeader($header)
      ->execute();

  $rows = array();
  $roles = user_roles();
  foreach ($results as $profile) {
    $type = is_null($profile->tid) ? t('All') : $profile->type;
    $severity = _watchdog_alert_get_severity_text($profile->severities);
    $ops = array(
      array(
        'title' => t('Edit'),
        'href' => 'admin/config/development/watchdog_alert/' . $profile->pid . '/edit',
      ),
      array(
        'title' => t('Delete'),
        'href' => 'admin/config/development/watchdog_alert/' . $profile->pid . '/delete',
      ),
    );
    $row = array(
      'data' => array(
        $type,
        $severity,
        $roles[$profile->rid],
        $profile->repeat_alert ? t('Yes') : t('No'),
        $profile->auto_dismiss ? t('Yes') : t('No'),
        $profile->dismiss_for_all ? t('Yes') : t('No'),
        theme('links__ctools_dropbutton', array('links' => $ops)),
      ),
    );
    $rows[] = $row;
  }
  $build = array(
    'watchdog_alert_table' => array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
      '#attributes' => array('id' => 'admin-dblog-alert'),
      '#empty' => t('There are no alerts set.'),
    ),
    'watchdog_alert_pager' => array('#theme' => 'pager'),
    'watchdog_alert_new' => drupal_get_form('_watchdog_alert_new_button_form'),
    'watchdog_main_settings' => system_settings_form(_watchdog_alert_main_settings_form()),
  );
  return $build;
}

/**
 * @return array
 *   The form elements for the main settings of watchdog alert for passing to
 *   system_settings_form()
 */
function _watchdog_alert_main_settings_form() {
  $_1_day = 24 * 60 * 60;
  $_2_days = 2 * 24 * 60 * 60;
  $_1_week = 7 * 24 * 60 * 60;
  $_2_weeks = 2 * 7 * 24 * 60 * 60;
  $_4_weeks = 4 * 7 * 24 * 60 * 60;
  $_6_months = 26 * 7 * 24 * 60 * 60;
  $_1_year = 365 * 24 * 60 * 60;
  return array(
    'watchdog_alert_exclude_user_1' => array(
      '#type' => 'checkbox',
      '#title' => t('Exclude user 1 from receiving alerts.'),
      '#description' => t('The default is to treat user 1 as belonging to all groups'),
      '#default_value' => variable_get('watchdog_alert_exclude_user_1', FALSE),
    ),
    'watchdog_alert_expire' => array(
      '#type' => 'select',
      '#title' => t('Automatically dismiss alerts older than...'),
      '#options' => array(
        0 => t('Never'),
        $_1_day => t('1 day'),
        $_2_days => t('2 days'),
        $_1_week => t('1 week'),
        $_2_weeks => t('2 weeks'),
        $_4_weeks => t('4 weeks'),
        $_6_months => t('6 months'),
        $_1_year => t('1 year'),
      ),
      '#default_value' => variable_get('watchdog_alert_expire', $_2_weeks),
    ),
    'watchdog_alert_store_max_user' => array(
      '#type' => 'select',
      '#title' => t('Limit the number of alerts per user to...'),
      '#description' => t('When there are alerts exceeding this limit, older alerts will be dismissed.'),
      '#options' => array(
        10 => t('10'),
        100 => t('100'),
        1000 => t('1, 000'),
        10000 => t('10, 000'),
        100000 => t('100, 000'),
        1000000 => t('1, 000, 000'),
      ),
      '#default_value' => variable_get('watchdog_alert_store_max_user', 100),
    ),
    'watchdog_alert_display_max' => array(
      '#type' => 'select',
      '#title' => t('Limit the number of alerts to display at once...'),
      '#description' => t('When there are alerts exceeding this limit, newer alerts will be hidden until older alerts are dismissed.'),
      '#options' => array(
        0 => t('No limit'),
        10 => t('10'),
        20 => t('20'),
        30 => t('30'),
        50 => t('50'),
        75 => t('75'),
        100 => t('100'),
      ),
      '#default_value' => variable_get('watchdog_alert_display_max', 10),
    ),
  );
}

/**
 * Form constructor for the button to launch creating a new alert profile.
 *
 * @see _watchdog_alert_new_button_form_submit()
 *
 * @ingroup forms
 */
function _watchdog_alert_new_button_form($form, $form_state) {
  return array(
    '#attributes' => array('class' => array('clearfix')),
    array(
      '#type' => 'submit',
      '#value' => 'Create New',
      '#attributes' => array('style' => 'float:right;'),
    ),
  );
}

/**
 * Form submission handler for _watchdog_alert_new_button_form()
 */
function _watchdog_alert_new_button_form_submit($form, $form_state) {
  drupal_goto('admin/config/development/watchdog_alert/new');
}

/**
 * Form constructor for the alert edit/create form.
 *
 * @param int $pid
 *   The profile id of the alert profile to edit, can be omitted (or set to
 *   NULL) to create a new alert profile.
 *
 * @see _watchdog_alert_edit_form_submit()
 *
 * @ingroup forms
 */
function _watchdog_alert_edit_form($form, $form_state, $pid = NULL) {
  if (!is_null($pid)) {
    $profile = db_select('watchdog_alert_profile', 'p')
        ->fields('p')
        ->condition('p.id', $pid)
        ->execute()
        ->fetchAssoc();
  }
  $severities = array();
  if (!empty($profile['severity'])) {
    $severities = _watchdog_alert_bitstring_to_array($profile['severity']);
  }
  $form = array(
    'type' => array(
      '#title' => t('Types'),
      '#type' => 'select',
      '#options' => watchdog_alert_get_types(),
      '#empty_option' => t('- All -'),
      '#default_value' => isset($profile['tid']) ? $profile['tid'] : '',
    ),
    'severity' => array(
      '#title' => t('Severities'),
      '#description' => t('Leaving this empty is equivalent to selecting all severities.'),
      '#type' => 'select',
      '#options' => watchdog_severity_levels(),
      '#multiple' => TRUE,
      '#size' => 8,
      '#default_value' => $severities,
    ),
    'role' => array(
      '#type' => 'select',
      '#title' => t('Role'),
      '#required' => TRUE,
      '#description' => t('You must select a role allowed to access site reports.'),
      '#options' => user_roles(TRUE, 'access site reports'),
      '#default_value' => isset($profile['rid']) ? $profile['rid'] : '',
      '#empty_option' => t('- Select -'),
    ),
    'repeat_alert' => array(
      '#type' => 'checkbox',
      '#title' => t('Show Repeated Alerts'),
      '#description' => t('By default multiple events for the same profile are collapsed to a single alert showing the number of events that occured, and information from the latest event.'),
      '#default_value' => empty($profile['repeat_alert']) ? 0 : 1,
    ),
    'auto_dismiss' => array(
      '#type' => 'checkbox',
      '#title' => t('Dismiss Automatically'),
      '#description' => t('If selected, the alert will be shown once and then automatically dismissed.'),
      '#default_value' => empty($profile['auto_dismiss']) ? 0 : 1,
    ),
    'dismiss_for_all' => array(
      '#type' => 'checkbox',
      '#title' => t('Dismiss for All'),
      '#description' => t('If selected, whenever an alert is dismissed it is dismissed for all users.'),
      '#default_value' => empty($profile['dismiss_for_all']) ? 0 : 1,
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    ),
    'cancel' => array(
      '#type' => 'submit',
      '#value' => t('Cancel'),
      '#submit' => array('_watchdog_alert_edit_form_submit'),
      '#limit_validation_errors' => array(),
    ),
  );
  if (isset($profile['pid'])) {
    $form['pid'] = array(
      '#type' => 'hidden',
      '#value' => $profile['pid'],
    );
  }
  return $form;
}

/**
 * Form submission handler for _watchdog_alert_edit_form()
 */
function _watchdog_alert_edit_form_submit(&$form, $form_state) {
  if ($form_state['triggering_element']['#parents'][0] == 'submit') {
    $values = $form_state['values'];
    $severities = _watchdog_alert_array_to_bitstring($values['severity']);
    if ($severities == 0) {
      $severities = WATCHDOG_ALERT_ALL_SEVERITIES;
    }
    $profile = array(
      'tid' => empty($values['type']) ? NULL : $values['type'],
      'severities' => $severities,
      'rid' => $values['role'],
      'repeat_alert' => empty($values['repeat_alert']) ? 0 : 1,
      'auto_dismiss' => empty($values['auto_dismiss']) ? 0 : 1,
      'dismiss_for_all' => empty($values['dismiss_for_all']) ? 0 : 1,
    );
    if (isset($values['pid'])) {
      $profile['pid'] = $values['pid'];
    }
    if (watchdog_alert_save_profile($profile)) {
      watchdog('watchdog_alert', t('The alert was saved successfully.'), array(), WATCHDOG_INFO);
    }
    else {
      watchdog('watchdog_alert', t('The alert could not be saved.'), array(), WATCHDOG_WARNING);
    }
  }
  drupal_goto('admin/config/development/watchdog_alert');
}

/**
 * Save an alert profile to the database.
 *
 * @param array $profile
 *   An associative array describing the alert profile containing:
 *   - pid: Optional.  If provided, this will update the alert profile with this
 *     id.  If omitted, a new alert profile will be created and assigned an id.
 *   - tid: The {watchdog_alert_type}.tid to filter for, or NULL for all types.
 *   - rid: The {role}.rid to whom alerts should be shown.
 *   - repeat_alert: Whether to display each instance of a repeated alert.
 *   - auto_dismiss: Whether to dismiss automaticallly after showing.
 *   - dismiss_for_all: Whether one user dissmissing the alert should dismiss it
 *     for everyone.
 *
 * @return bool
 *   TRUE for success
 */
function watchdog_alert_save_profile($profile) {
  if (isset($profile['pid'])) {
    $query = db_update('watchdog_alert_profile')
        ->condition('pid', intval($profile['pid']));
    unset($profile['pid']);
  }
  else {
    $query = db_insert('watchdog_alert_profile');
  }
  $result = $query->fields($profile)
      ->execute();
  /*
   * This does not type check since InsertQuery::execute() and
   * UpdateQuery::execute() have different return values, but both evaluate to
   * FALSE on failure.
   */
  return $result != FALSE;
}

/**
 * Form constructor for the alert profile delete form.
 *
 * Verify deletion.
 *
 * @param int $pid
 *   The profile id of the alert profile to delete.
 *
 * @see _watchdog_alert_delete_form_submit()
 *
 * @ingroup forms
 */
function _watchdog_alert_delete_form($form, $form_state, $pid) {
  $form = array(
    'pid' => array(
      '#type' => 'hidden',
      '#value' => $pid,
    ),
  );
  $question = t('Are you sure you want to delete this alert profile?');
  $path = 'admin/config/development/watchdog_alert';
  $description = t('This action cannot be undone.');
  $yes = t('Delete');
  $no = t('Cancel');
  return confirm_form($form, $question, $path, $description, $yes, $no);
}

/**
 * Form submission handler for _watchdog_alert_delete_form()
 */
function _watchdog_alert_delete_form_submit(&$form, $form_state) {
  watchdog_alert_delete_profile($form_state['values']['pid']);
  drupal_goto('admin/config/development/watchdog_alert');
}

/**
 * Delete an alert profile from the database
 *
 * @param int $pid
 *   The alert id
 */
function watchdog_alert_delete_profile($pid) {
  db_delete('watchdog_alert_profile')
      ->condition('pid', $pid)
      ->execute();
}

/**
 * Build an alert message and display it to the user.  If it's set to
 * automatically dismiss, dismiss it.
 *
 * @param object $alert
 *   The alert object returned from the database
 */
function _watchdog_alert_show_alert($alert) {
  if ($alert->auto_dismiss) {
    $alert_message = t('Database Logging Alert: !message', array(
      '!message' => theme('watchdog_alert_message', array('alert' => $alert)),
    ));
    _watchdog_alert_dismiss($alert);
  }
  else {
    $dismiss_path = 'admin/config/development/watchdog_alert/'
        . $alert->aid . '/dismiss';
    $dismiss_options = array(
      'query' => array('destination' => current_path()),
    );
    $dismiss_link = l(t('Dismiss'), $dismiss_path, $dismiss_options);
    $alert_message = t('[!dismiss] Database Logging Alert: !message', array(
      '!message' => theme('watchdog_alert_message', array('alert' => $alert)),
      '!dismiss' => $dismiss_link,
    ));
  }
  $type = _watchdog_alert_severity_to_type($alert->severity);
  drupal_set_message($alert_message, $type);
}

/**
 * @param int $severity
 *   A watchdog severity level
 *
 * @return string
 *   A corresponding $type for drupal_set_message()
 */
function _watchdog_alert_severity_to_type($severity) {
  switch ($severity) {
    case WATCHDOG_EMERGENCY:
    case WATCHDOG_ALERT:
    case WATCHDOG_CRITICAL:
    case WATCHDOG_ERROR:
      return 'error';
    case WATCHDOG_WARNING:
      return 'warning';
    case WATCHDOG_NOTICE:
    case WATCHDOG_INFO:
    case WATCHDOG_DEBUG:
    default:
      return 'status';
  }
}

/**
 * Dismisss an alert by removing it from the database.
 *
 * @param int $aid
 *   The alert id
 */
function _watchdog_alert_dismiss($aid) {
  db_delete('watchdog_alert')
      ->condition('aid', $aid)
      ->execute();
}

/**
 * Page callback: Dismiss an alert
 *
 * @param int $aid
 *   The alert id
 * @param int $wid
 *   The watchdog id
 */
function _watchdog_alert_dismiss_page($aid) {
  _watchdog_alert_dismiss($aid);
  drupal_goto(drupal_get_destination());
}

/**
 * @param int $uid
 *   Optional.  If given, filters the alerts to those that apply to the given
 *   user.
 *
 * @return array
 *   An array of alert objects
 */
function watchdog_alert_get_alerts($uid = Null) {
  $query = db_select('watchdog_alert', 'a');
  $query->join('watchdog_alert_profile', 'p', 'a.pid = p.pid');
  $query->join('watchdog_alert_type', 't', 'a.tid  = t.tid');
  $query->fields('a')
      ->fields('p')
      ->fields('a');
  if (!is_null($uid)) {
    $user = user_load($uid);
    $roles = array_keys($user->roles);
    $query->condition(db_or()
            ->condition('a.uid', $uid)
            ->condition(db_and()
                ->condition('a.uid', 0)
                ->condition('p.rid', $roles)
            )
    );
  }
  return $query->execute()->fetchAll();
}

/**
 * Remove alerts from the table which were generated longer ago than the
 * configured time.
 */
function _watchdog_alert_remove_expired() {
  $_2_weeks = 2 * 7 * 24 * 60 * 60;
  $expire = variable_get('watchdog_alert_expire', $_2_weeks);
  if ($expire == 0) {
    return;
  }
  db_delete('watchdog_alert')
      ->condition('timestamp', time() - $expire, '<')
      ->execute();
}

/**
 * Remove alerts from the table which were generated for users who no longer
 * have the 'access site reports' permission.
 */
function _watchdog_alert_remove_no_access() {
  $users_with_access = db_select('users', 'u')
      ->fields('u', array('uid'));
  $users_with_access->leftJoin('users_roles', 'r', 'u.uid = r.uid');
  $users_with_access->leftJoin('role_permission', 'p', 'r.rid = p.rid');
  $users_with_access->condition('p.permission', 'access site reports');
  db_delete('watchdog_alert')
      ->condition('uid', $users_with_access, 'NOT IN')
      ->condition('uid', 1, "!=")
      ->execute();
}

/**
 * Remove alerts from the table which exceed the configured per user alert
 * limit.
 *
 * @todo Actually do this.
 */
function _watchdog_alert_remove_over_limit() {
  $limit = variable_get('watchdog_alert_store_max_user', 100);
}

/**
 * @param int $severities
 *   A severities bitstring
 * 
 * @return string
 *   A string containing a user-friendly representation of the set of severity
 *   levels.  It may be plain text or an html list.
 */
function _watchdog_alert_get_severity_text($severities) {
  if ($severities == WATCHDOG_ALERT_ALL_SEVERITIES) {
    return t('All');
  }
  $texts = array();
  foreach (watchdog_severity_levels() as $severity => $severity_text) {
    $all_higher = pow(2, $severity + 1) - 1;
    $all_lower = WATCHDOG_ALERT_ALL_SEVERITIES - pow(2, $severity) - 1;
    if ($severity != WATCHDOG_EMERGENCY && $severities == $all_higher) {
      return t('@severity or higher', array('@severity' => $severity_text));
    }
    elseif ($severity != WATCHDOG_DEBUG && $severities == $all_lower) {
      return t('@severity or lower', array('@severity' => $severity_text));
    }
    if ($severities & pow(2, $severity)) {
      $texts[] = $severity_text;
    }
  }
  if (count($texts) == 1) {
    return $texts[0];
  }
  if (isset($severity)) {
    return $severity;
  }
  return theme_item_list(array('items' => $texts));
}

/**
 * Convert a severity bitstring to a severity array
 *
 * @param int $severity_bitstring
 *   A severity bitstring: each bit reprents inclusion of that severity
 *
 * @return int[]
 *   The corresponding severity array: an array of included severity values
 *
 * @see _watchdog_alert_array_to_bitstring()
 */
function _watchdog_alert_bitstring_to_array($severity_bitstring) {
  $severity_array = array();
  foreach (array_keys(watchdog_severity_levels()) as $severity) {
    if ($severity_bitstring & pow(2, $severity)) {
      $severity_array[] = $severity;
    }
  }
  return $severity_array;
}

/**
 * Convert a severity array to a severity bitstring
 *
 * @param int[] $severity_array
 *   A severity array: an array of included severity values
 *
 * @return int
 *   The corresponding severity bitstring: each bit reprents inclusion of that
 *   severity
 *
 * @see _watchdog_alert_bitstring_to_array()
 */
function _watchdog_alert_array_to_bitstring($severity_array) {
  $severity_bitstring = 0;
  foreach ($severity_array as $severity) {
    $severity_bitstring += pow(2, $severity);
  }
  return $severity_bitstring;
}

/**
 * Returns the HTML for an alert message.
 *
 * @param array $variables
 *   An associative array containing only:
 *   - alert: An alert object
 *
 * @ingroup themeable
 */
function theme_watchdog_alert_message($variables) {
  $alert = $variables['alert'];
  if (is_null($alert)) {
    return '';
  }
  $output = '<span class="watchdog-alert-severity-' . $variables->severity . '">';
  if($alert->count > 1){
    $output .= '<span class="watchdog-alert-count">(' . $alert->count . ')</span> ';
  }
  $output .= '<span class="watchdog-alert-type">' . $alert->type . '</span>' . ": ";
  if (is_null(unserialize($alert->variables))) {
    $output .= $alert->message;
  }
  else {
    $output .= t($alert->message, unserialize($alert->variables));
  }
  $output .= '</span>';
  return $output;
}

/**
 * @param string $type
 *   The type to match
 * @param int $severity
 *   The severity to match
 *
 * @return object[]
 *   An array of profile objects that match the parameters given
 */
function _watchdog_alert_get_profiles($type, $severity) {
  $tid = db_select('watchdog_alert_type', 't')
      ->fields('t', array('tid'))
      ->condition('type', $type);
  $profiles = db_select('watchdog_alert_profile', 'p')
      ->fields('p')
      ->condition(db_or()
          ->condition('p.tid', $tid)
          ->isNull('p.tid')
      )
      ->condition('severities', pow(2, $severity), '&')
      ->execute()
      ->fetchAll();
  return $profiles;
}

/**
 * @param int $rid
 *   The role id
 *
 * @return int[]
 *   An array of uids that have the role specified
 */
function _watchdog_alert_get_uids_by_rid($rid) {
  return db_select('users_roles', 'ur')
          ->fields('ur', array('uid'))
          ->distinct()
          ->condition('rid', $rid)
          ->execute()
          ->fetchCol();
}

/**
 * Save an alert to the databse. If $repeat_alert is true, a new alert will
 * always be generated.  Otherwise, a new alert will be generated only if there
 * is not already an alert matching both $uid and $pid.  If such an alert exists
 * it will be updated according to $log_entry, and its count will be
 * incremented.
 *
 * @param array $log_entry
 *   A log entry array from hook_watchdog().
 * @param int $uid
 *   The id of the user to whom the message should be shown.  A value of 0 is
 *   used to show this alert to all members of the role associated with the
 *   alert profile, which means that any member can dismiss it for everyone.
 * @param int $pid
 *   The id of the profile generating this alert.
 * @param bool $repeat_alert
 *
 */
function _watchdog_alert_save_alert($log_entry, $uid, $pid, $repeat_alert) {
  $tid = db_select('watchdog_alert_type', 't')
      ->fields('t', array('tid'))
      ->condition('type', $log_entry['type'])
      ->execute()
      ->fetchField();
  $alert = array(
    'pid' => $pid,
    'uid' => $uid,
    'tid' => $tid,
    'message' => $log_entry['message'],
    'variables' => serialize($log_entry['variables']),
    'severity' => $log_entry['severity'],
    'link' => substr($log_entry['link'], 0, 255),
    'trigger_uid' => $log_entry['uid'],
    'request_uri' => $log_entry['request_uri'],
    'referer' => $log_entry['referer'],
    'ip' => substr($log_entry['ip'], 0, 128),
    'timestamp' => $log_entry['timestamp'],
  );
  if ($repeat_alert) {
    db_insert('watchdog_alert', 'a')
        ->fields($alert)
        ->execute();
  }
  else {
    db_merge('watchdog_alert', 'a')
        ->key(array('pid' => $pid, 'uid' => $uid))
        ->fields($alert)
        ->expression('count', 'count + :inc', array(':inc' => 1))
        ->execute();
  }
}
