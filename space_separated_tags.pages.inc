<?php
/**
 * @file
 */

/**
 * Page callback: Outputs JSON for taxonomy autocomplete suggestions.
 *
 * Path: space_separated_tags_taxonomy/autocomplete
 *
 * This callback outputs term name suggestions in response to Ajax requests
 * made by the taxonomy autocomplete widget for taxonomy term reference
 * fields. The output is a JSON object of plain-text term suggestions, keyed by
 * the user-entered value with the completed term name appended.  Term names
 * containing commas are wrapped in quotes.
 *
 * For example, suppose the user has entered the string 'red fish, blue' in the
 * field, and there are two taxonomy terms, 'blue fish' and 'blue moon'. The
 * JSON output would have the following structure:
 * @code
 *   {
 *     "red fish, blue fish": "blue fish",
 *     "red fish, blue moon": "blue moon",
 *   };
 * @endcode
 *
 * @param $field_name
 *   The name of the term reference field.
 * @param $tags_typed
 *   (optional) A comma-separated list of term names entered in the
 *   autocomplete form element. Only the last term is used for autocompletion.
 *   Defaults to '' (an empty string).
 *
 * @see taxonomy_menu()
 * @see taxonomy_field_widget_info()
 */
function space_separated_tags_taxonomy_autocomplete($field_name, $tags_typed = '') {
  // If the request has a '/' in the search text, then the menu system will have
  // split it into multiple arguments, recover the intended $tags_typed.
  $args = func_get_args();
  // Shift off the $field_name argument.
  array_shift($args);
  $tags_typed = implode('/', $args);

  // Make sure the field exists and is a taxonomy field.
  if (!($field = field_info_field($field_name)) || $field['type'] !== 'taxonomy_term_reference') {
    // Error string. The JavaScript handler will realize this is not JSON and
    // will display it as debugging information.
    print t('Taxonomy field @field_name not found.', array('@field_name' => $field_name));
    exit;
  }

  // The user enters a comma-separated list of tags. We only autocomplete the last tag.
  $tags_typed = space_separated_tags_explode_tags($tags_typed);
  $tag_last = drupal_strtolower(array_pop($tags_typed));

  $matches = array();
  if ($tag_last != '') {

    // Part of the criteria for the query come from the field's own settings.
    $vids = array();
    $vocabularies = taxonomy_vocabulary_get_names();
    foreach ($field['settings']['allowed_values'] as $tree) {
      $vids[] = $vocabularies[$tree['vocabulary']]->vid;
    }

    $query = db_select('taxonomy_term_data', 't');
    $query->addTag('translatable');
    $query->addTag('term_access');

    // Do not select already entered terms.
    if (!empty($tags_typed)) {
      $query->condition('t.name', $tags_typed, 'NOT IN');
    }
    // Select rows that match by term name.
    $tags_return = $query
      ->fields('t', array('tid', 'name'))
      ->condition('t.vid', $vids)
      ->condition('t.name', '%' . db_like($tag_last) . '%', 'LIKE')
      ->range(0, 10)
      ->execute()
      ->fetchAllKeyed();

    $prefix = count($tags_typed) ? space_separated_tags_implode_tags($tags_typed) . ' ' : '';

    $term_matches = array();
    foreach ($tags_return as $tid => $name) {
      $n = $name;
      // Term names containing commas or quotes must be wrapped in quotes.
      if (strpos($name, ',') !== FALSE || strpos($name, '"') !== FALSE) {
        $n = '"' . str_replace('"', '""', $name) . '"';
      }
      $term_matches[$prefix . $n] = check_plain($name);
    }
  }

  drupal_json_output($term_matches);
}

/**
 * Implodes an array of tags into a string.
 *
 * @see drupal_explode_tags()
 */
function space_separated_tags_implode_tags($tags) {
  $encoded_tags = array();
  foreach ($tags as $tag) {
    // Commas and quotes in tag names are special cases, so encode them.
    if (strpos($tag, ',') !== FALSE || strpos($tag, '"') !== FALSE) {
      $tag = '"' . str_replace('"', '""', $tag) . '"';
    }

    $encoded_tags[] = $tag;
  }
  return implode(' ', $encoded_tags);
}
