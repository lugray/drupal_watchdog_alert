<?php

/**
 * @file
 * Provide callback functions for the administrative interface for the Database
 * Logging Alert module
 */

/**
 * Page callback: Module settings
 *
 * Display a table of existing alerts, giving the option to edit/delete them,
 * as well as a button to create a new alert.
 *
 * @see dblog_alert_menu()
 */
function _dblog_alert_settings() {
  ctools_include('dropbutton.theme');
  module_load_include('inc', 'dblog', 'dblog.admin');
  $header = array();
  foreach (dblog_filters() as $filter) {
    $header[] = $filter['title'];
  }
  $header = array_merge($header, array(
    array('data' => t('Role'), 'field' => 'daa.role', 'sort' => 'asc'),
    array('data' => t('Show Repeated Alerts'), 'field' => 'daa.repeat_alert'),
    array('data' => t('Dismiss Automatically'), 'field' => 'daa.auto_dismiss'),
    array('data' => t('Dismiss for All'), 'field' => 'daa.dismiss_for_all'),
    t('Operations'),
  ));

  $results = db_select('dblog_alert_alerts', 'daa')
      ->extend('PagerDefault')
      ->extend('TableSort')
      ->fields('daa')
      ->limit(50)
      ->orderByHeader($header)
      ->execute();

  $rows = array();
  $roles = user_roles();
  foreach ($results as $alert) {
    $filter = unserialize($alert->filter);
    $ops = array(
      array(
        'title' => t('Edit'),
        'href' => 'admin/config/development/dblog_alert/' . $alert->aid . '/edit',
      ),
      array(
        'title' => t('Delete'),
        'href' => 'admin/config/development/dblog_alert/' . $alert->aid . '/delete',
      ),
    );
    $row = array('data' => array());
    foreach (dblog_filters() as $key => $info) {
      if (empty($filter[$key])) {
        $row['data'][] = t('All');
      }
      else {
        $values = array();
        foreach ($filter[$key] as $option) {
          $values[] = $info['options'][$option];
        }
        $row['data'][] = implode(', ', $values);
      }
    }
    $row['data'] = array_merge($row['data'], array(
      $roles[$alert->role],
      $alert->repeat_alert ? t('Yes') : t('No'),
      $alert->auto_dismiss ? t('Yes') : t('No'),
      $alert->dismiss_for_all ? t('Yes') : t('No'),
      theme('links__ctools_dropbutton', array('links' => $ops)),
    ));
    $rows[] = $row;
  }
  $build = array();
  $build['dblog_alert_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#attributes' => array('id' => 'admin-dblog-alert'),
    '#empty' => t('There are no alerts set.'),
  );
  $build['dblog_alert_pager'] = array('#theme' => 'pager');
  $build['dblog_alert_new'] = drupal_get_form('_dblog_alert_new_button_form');
  return $build;
}

/**
 * Form constructor for the button to launch creating a new alert.
 *
 * @see _dblog_alert_new_button_form_submit()
 *
 * @ingroup forms
 */
function _dblog_alert_new_button_form($form, $form_state) {
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
 * Form submission handler for _dblog_alert_new_button_form()
 */
function _dblog_alert_new_button_form_submit($form, $form_state) {
  drupal_goto('admin/config/development/dblog_alert/new');
}

/**
 * Form constructor for the alert edit/create form.
 *
 * @param int $aid
 *   The alert id of the alert to edit, can be omitted (or set to NULL) to
 *   create a new alert.
 *
 * @see _dblog_alert_edit_form_submit()
 *
 * @ingroup forms
 */
function _dblog_alert_edit_form($form, $form_state, $aid = NULL) {
  module_load_include('inc', 'dblog', 'dblog.admin');
  if (!is_null($aid)) {
    $alert = db_select('dblog_alert_alerts', 'daa')
        ->fields('daa')
        ->condition('daa.aid', $aid)
        ->execute()
        ->fetchAssoc();
    if (!empty($alert)) {
      $alert['filter'] = unserialize($alert['filter']);
    }
  }
  $form = array();
  if (isset($alert['aid'])) {
    $form['aid'] = array(
      '#type' => 'hidden',
      '#value' => $alert['aid'],
    );
  }
  $form['filter'] = array(
    '#type' => 'fieldset',
    '#title' => 'Filters',
    '#tree' => TRUE,
    '#attributes' => array('class' => array('clearfix')),
  );
  foreach (dblog_filters() as $key => $filter) {
    $form['filter'][$key] = array(
      '#title' => $filter['title'],
      '#type' => 'select',
      '#options' => $filter['options'],
      '#multiple' => TRUE,
      '#size' => 8,
      '#prefix' => '<div style="float:left;margin-right:1em;">',
      '#suffix' => '</div>',
      '#default_value' => isset($alert['filter'][$key]) ? $alert['filter'][$key] : array(),
    );
  }
  $form+= array(
    'role' => array(
      '#type' => 'select',
      '#title' => t('Role'),
      '#required' => TRUE,
      '#description' => t('You must select a role allowed to access site reports.'),
      '#options' => user_roles(TRUE, 'access site reports'),
      '#default_value' => isset($alert['role']) ? $alert['role'] : '',
      '#empty_option' => t('- Select -'),
    ),
    'repeat_alert' => array(
      '#type' => 'checkbox',
      '#title' => t('Show Repeated Alerts'),
      '#description' => t('By default multiple events of the same kind with the same message are collapsed to a single alert.'),
      '#default_value' => empty($alert['repeat_alert']) ? 0 : 1,
    ),
    'auto_dismiss' => array(
      '#type' => 'checkbox',
      '#title' => t('Dismiss Automatically'),
      '#description' => t('If selected, the alert will be shown once and then automatically dismissed.'),
      '#default_value' => empty($alert['auto_dismiss']) ? 0 : 1,
    ),
    'dismiss_for_all' => array(
      '#type' => 'checkbox',
      '#title' => t('Dismiss for All'),
      '#description' => t('If selected, whenever an alert is dismissed it is dismissed for all users.'),
      '#default_value' => empty($alert['dismiss_for_all']) ? 0 : 1,
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    ),
    'cancel' => array(
      '#type' => 'submit',
      '#value' => t('Cancel'),
      '#submit' => array('_dblog_alert_edit_form_submit'),
      '#limit_validation_errors' => array(),
    ),
  );
  return $form;
}

/**
 * Form submission handler for _dblog_alert_edit_form()
 */
function _dblog_alert_edit_form_submit(&$form, $form_state) {
  if ($form_state['triggering_element']['#parents'][0] == 'submit') {
    $values = $form_state['values'];
    $alert = array(
      'filter' => serialize($values['filter']),
      'role' => intval($values['role']),
      'repeat_alert' => empty($values['repeat_alert']) ? 0 : 1,
      'auto_dismiss' => empty($values['auto_dismiss']) ? 0 : 1,
      'dismiss_for_all' => empty($values['dismiss_for_all']) ? 0 : 1,
    );
    if (isset($values['aid'])) {
      $alert['aid'] = $values['aid'];
    }
    if (dblog_alert_save_alert($alert)) {
      watchdog('dblog_alert', t('The alert was saved successfully.'), array(), WATCHDOG_INFO);
    }
    else {
      watchdog('dblog_alert', t('The alert could not be saved.'), array(), WATCHDOG_WARNING);
    }
  }
  drupal_goto('admin/config/development/dblog_alert');
}

/**
 * Save an alert to the database.
 *
 * @param array $alert
 *   An array describing the alert with keys:
 *   - aid: Optional.  If provided, this will update the alert with this id.  If
 *     omitted, a new alert will be created and assigned an id.
 *   - filter: A serialized array representing the filter.
 *   - role: The role to whom alerts should be shown.
 *   - repeat_alert: Whether to display each instance of a repeated alert.
 *   - auto_dismiss: Whether to dismiss automaticallly after showing.
 *   - dismiss_for_all: Whether one user dissmissing the alert should dismiss it
 *     for everyone.
 *
 * @return boolean
 *   TRUE for success
 */
function dblog_alert_save_alert($alert) {
  if (isset($alert['aid'])) {
    $query = db_update('dblog_alert_alerts')
        ->condition('aid', intval($alert['aid']));
    unset($alert['aid']);
  }
  else {
    $query = db_insert('dblog_alert_alerts');
  }
  $result = $query->fields($alert)
      ->execute();
  /*
   * This does not type check since InsertQuery::execute() and
   * UpdateQuery::execute() have different return values, but both evaluate to
   * FALSE on failure.
   */
  return $result != FALSE;
}

/**
 * Form constructor for the alert delete form.
 *
 * Verify deletion.
 *
 * @param int $aid
 *   The alert id of the alert to delete.
 *
 * @see _dblog_alert_delete_form_submit()
 *
 * @ingroup forms
 */
function _dblog_alert_delete_form($form, $form_state, $aid) {
  $form = array(
    'aid' => array(
      '#type' => 'hidden',
      '#value' => $aid,
    ),
  );
  $question = t('Are you sure you want to delete this alert?');
  $path = 'admin/config/development/dblog_alert';
  $description = t('This action cannot be undone.');
  $yes = t('Delete');
  $no = t('Cancel');
  return confirm_form($form, $question, $path, $description, $yes, $no);
}

/**
 * Form submission handler for _dblog_alert_delete_form()
 */
function _dblog_alert_delete_form_submit(&$form, $form_state) {
  dblog_alert_delete_alert($form_state['values']['aid']);
  drupal_goto('admin/config/development/dblog_alert');
}

/**
 * Delete an alert from the database
 *
 * @param int $aid
 *   The alert id
 */
function dblog_alert_delete_alert($aid) {
  db_delete('dblog_alert_alerts')
      ->condition('aid', intval($aid))
      ->execute();
}

/**
 * Build an alert message and display it to the user if it's a new message or
 * the alert is set to repeat.
 *
 * @param object $alert
 *   The alert object returned from the database
 * @param object $dblog
 *   The dblog object returned from the database
 */
function _dblog_alert_set_alert($alert, $dblog) {
  module_load_include('admin.inc', 'dblog');
  if (!$alert->repeat_alert) {
    $displayed_alerts = &drupal_static(__FUNCTION__, array());
    if (!isset($displayed_alerts[$alert->aid])) {
      $displayed_alerts[$alert->aid] = array();
    }
    if (in_array($dblog->message, $displayed_alerts[$alert->aid])) {
      return;
    }
    $displayed_alerts[$alert->aid][] = $dblog->message;
  }
  if ($alert->auto_dismiss) {
    $alert_message = t('Database Logging Alert: !message', array(
      '!message' => theme('dblog_message', array('event' => $dblog, 'link' => TRUE)),
    ));
    _dblog_alert_dismiss($alert, $dblog->wid);
  }
  else {
    $dismiss_path = 'admin/config/development/dblog_alert/dismiss/'
        . $alert->aid . "/" . $dblog->wid;
    $dismiss_link = l(t('Dismiss'), $dismiss_path, array('query' => array('destination' => current_path())));
    $alert_message = t('[!dismiss] Database Logging Alert: !message', array(
      '!message' => theme('dblog_message', array('event' => $dblog, 'link' => TRUE)),
      '!dismiss' => $dismiss_link,
    ));
  }
  $type = _dblog_alert_severity_to_type($dblog->severity);
  drupal_set_message($alert_message, $type);
}

/**
 * @param int $severity
 *   A watchdog severity level
 *
 * @return string
 *   A corresponding $type for drupal_set_message()
 */
function _dblog_alert_severity_to_type($severity) {
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
 * Dismisss an alert.  If repeated alerts are not shown, dismiss all alerts
 * which display with this one.
 *
 * @param mixed $alert
 *   The alert object or id
 * @param int $wid
 *   The watchdog id
 */
function _dblog_alert_dismiss($alert, $wid) {
  global $user;
  if (!is_object($alert)) {
    $alert = db_select('dblog_alert_alerts', 'daa')
            ->fields('daa', array('aid', 'repeat_alert'))
            ->condition('aid', intval($alert))
            ->execute()->fetch();
  }
  if ($alert->repeat_alert) {
    $wids = array($wid);
  }
  else {
    $message = db_select('watchdog', 'w')
        ->fields('w', array('message'))
        ->condition('w.wid', $wid);
    $wids = db_select('watchdog', 'w')
        ->fields('w', array('wid'))
        ->condition('w.message', $message, 'IN')
        ->execute()
        ->fetchCol();
  }
  $dismiss = db_insert('dblog_alert_dismissed')
      ->fields(array('uid', 'wid', 'aid'));
  foreach ($wids as $wid) {
    $dismiss->values(array('uid' => $user->uid, 'wid' => $wid, 'aid' => $alert->aid));
  }
  $dismiss->execute();
}

/**
 * Page callback: Dismiss an alert
 *
 * @param int $aid
 *   The alert id
 * @param int $wid
 *   The watchdog id
 */
function _dblog_alert_dismiss_page($aid, $wid) {
  _dblog_alert_dismiss($aid, $wid);
  drupal_goto(drupal_get_destination());
}

/**
 * @param int[] $roles
 *   Optional.  If given, filters the alerts to those that apply to the given
 *   list of roles.
 *
 * @return array
 *   An array of alert objects
 */
function dblog_alert_get_alerts($roles = Null) {
  $query = db_select('dblog_alert_alerts', 'daa')
      ->fields('daa');
  if (!is_null($roles)) {
    $query->condition('daa.role', $roles, 'IN');
  }
  return $query->execute()->fetchAll();
}
