<?php
// $Id: welcome.inc,v 1.1.2.7.2.17.2.12 2010/09/02 10:21:50 goba Exp $

/**
 * @file
 *   Introduction page for an l10n_community setup.
 */

/**
 * Introduction page for an l10n_community setup.
 *
 * All string are in PHP source code, so translators can translate
 * them to their own language, without resorting to i18n module or
 * some other solution for node translation.
 *
 * This page is based on the admin page layout from system.module,
 * so we are using classes and concepts from there. This might not
 * fit perfectly with all listings, so I bended some stuff more to comply.
 */
function l10n_community_welcome_page() {
  if ($project = l10n_community_get_highlighted_project()) {
    // Display progress status of the highlighted project.
    include_once drupal_get_path('module', 'l10n_community') .'/pages.inc';
    $output = theme('admin_block', l10n_community_language_progress_for_project($project, l10n_community_get_languages(), t('Translation status for %project', array('%project' => $project->title)), t('Highlighted project')));
  }
  else {
    $output = t('No highlighted project set.');
  }
  return $output;
}

/**
 * Block builder function to generate the main contribution block.
 */
function l10n_community_block_contribute_front() {
  // Build contribution call to action block.
  $content = '';
  if (!$GLOBALS['user']->uid) {
    // Only display account creation task if not logged in.
    $content = '<div class="l10n-next-action">'. l(t('Register'), 'user/register') .'</div>';
    $content .= '<p>'. t('You can review the work being done here, but to participate, first you need to <a href="@register">create an account and/or log in</a>.', array('@register' => url('user/login'))) .'</p>';
  }
  $content .= drupal_get_form('l10n_community_pick_go') .'<div class="clearing"></div>';
  return array(
    'subject' => t('Contribute'),
    'content' => $content
  );
}

function l10n_community_pick_go(&$form_state) {
  $form = array();
  $languages = l10n_community_get_languages();
  $projects = l10n_server_get_projects();

  // Set default project based on possible URL argument (eg. used on downloads page).
  if (empty($form_state['values']['project']) && !empty($_GET['project']) && isset($projects[$_GET['project']])) {
    $form_state['values']['project'] = $projects[$_GET['project']]->title;
  }

  // Drop languages without a plural formula from the options.
  $language_list = array();
  foreach ($languages as $langcode => $language) {
    if (!empty($language->plurals)) {
      $language_list[$langcode] = t($language->name);
    }
  }

  if (!count($language_list) || !count($projects)) {
    $pconfig_tip = '';
    if (!module_exists('l10n_pconfig')) {
      $pconfig_tip = ' '. t('The <a href="@pconfig">Plural formula configurator</a> module is recommended for plural formula setup.', array('@pconfig' => 'http://drupal.org/project/l10n_pconfig'));
    }
    $form['dummy'] = array('#type' => 'markup', '#value' => t('No languages with plural formulas defined or no projects scanned yet. Check the <a href="@languages">configuration of languages</a>, <a href="@connectors">list of connectors</a> and <a href="@projects">projects</a>.', array('@languages' => url('admin/settings/language'), '@connectors' => url('admin/l10n_server/connectors'), '@projects' => url('admin/l10n_server/projects'))) . $pconfig_tip);
    return $form;
  }

  if (module_exists('l10n_groups')) {
    // We are dealing with a groups based permission model.
    $form['groups'] = l10n_groups_pick_go();
  }

  if (count($language_list) > 1) {
    // If more then one language.
    $form['langcode'] = array(
      // For three or less languages, display radio buttons, select otherwise.
      '#type' => (count($language_list) <= 3) ? 'radios' : 'select',
      '#title' => t('Pick a language'),
      '#options' => array('' => t('All')) + $language_list,
      '#description' => t('Alternatively you can <a href="@explore">explore languages</a>.', array('@explore' => url('translate/languages')))
    );
  }
  else {
    // Only one language, store as form value.
    list($language_code, $language_name) = each($language_list);
    $form['langcode'] = array(
      '#type' => 'value',
      '#value' => $language_code
    );
  }

  if (count($language_list) > 1) {
    $projects_title = user_access('browse translations') ? t('And/or pick a project') : t('Or pick a project');
  }
  else {
    $projects_title = t('Pick a project for @language translation', array('@language' => $language_name));
  }
  $form['project'] = array(
    '#title' => $projects_title,
    '#description' => t('Alternatively you can <a href="@explore">explore projects</a>.', array('@explore' => url('translate/projects'))),
    '#default_value' => $form_state['values']['project'],
  );
  if (($count = count($projects)) <= 30) {
    // Radio box widget for as much as 5 projects, select widget for 5-30 projects.
    $form['project']['#type'] = ($count <= 5 ? 'radios' : 'select');
    $form['project']['#options'] = array('' => t('All'));
    foreach ($projects as $project) {
      // title used to conform to the autocomplete behavior.
      $form['project']['#options'][$project->title] = $project->title;
    }
  }
  else {
    // Autocomplete field for more then 30 projects.
    $form['project'] += array(
      '#type' => 'textfield',
      '#autocomplete_path' => 'translate/project-autocomplete',
    );
  }

  // Either we have languages or projects, so we can jump.
  $form['submit'] = array(
    '#type' => 'submit',
    '#prefix' => '<div class="clearing"></div>',
    '#value' => t('Choose'),
  );
  return $form;
}

function l10n_community_pick_go_validate($form, &$form_state) {
  if (empty($form_state['values']['project']) && empty($form_state['values']['langcode'])) {
    // We require at least one to be selected to be effective.
    form_set_error('langcode', t('At least select a language or a project.'));
  }
}

/**
 * Submission handler for l10n_community_pick_go().
 *
 * Redirects the user to the selected project page with language selection
 * if both are provided, or to the project page with project only or the
 * language page with language only.
 */
function l10n_community_pick_go_submit($form, &$form_state) {
  if (!empty($form_state['values']['project']) && ($uri = l10n_community_project_uri_by_title($form_state['values']['project']))) {
    // Project surely selected, possibly with language too.
    if (!empty($form_state['values']['langcode'])) {
      if (user_access('browse translations')) {
        drupal_goto('translate/languages/'. $form_state['values']['langcode'] .'/translate', 'project='. $uri);
      }
      else {
        // Both language and project set, no access to browse > go to project.
        drupal_goto('translate/projects/'. $uri);
      }
    }
    else {
      // Only project set.
      drupal_goto('translate/projects/'. $uri);
    }
  }
  elseif (!empty($form_state['values']['langcode'])) {
    // Only language set.
    drupal_goto('translate/languages/'. $form_state['values']['langcode']);
  }
}
