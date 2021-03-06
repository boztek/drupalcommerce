<?php
// $Id$

/**
 * @file
 * Builds placeholder replacement tokens for line item related data.
 */


/**
 * Implements hook_token_info().
 */
function commerce_line_item_token_info() {
  $type = array(
    'name' => t('Line items'),
    'description' => t('Tokens related to individual line items.'),
    'needs-data' => 'line_item',
  );

  // Tokens for line items.
  $line_item = array();

  $line_item['line-item-id'] = array(
    'name' => t('Line item ID'),
    'description' => t('The unique numeric ID of the line item.'),
  );
  $line_item['line-item-label'] = array(
    'name' => t('Line item label'),
    'description' => t('The label displayed with the line item.'),
  );
  $line_item['quantity'] = array(
    'name' => t('Quantity'),
    'description' => t('The quantity of the line item.'),
  );

  // Chained tokens for products.
  $line_item['created'] = array(
    'name' => t('Date created'),
    'description' => t('The date the line item was created.'),
    'type' => 'date',
  );
  $line_item['changed'] = array(
    'name' => t('Date updated'),
    'description' => t('The date the line item was last updated.'),
    'type' => 'date',
  );

  return array(
    'types' => array('line_item' => $type),
    'tokens' => array('line_item' => $line_item),
  );
}

/**
 * Implements hook_tokens().
 */
function commerce_line_item_tokens($type, $tokens, array $data = array(), array $options = array()) {
  $url_options = array('absolute' => TRUE);

  if (isset($options['language'])) {
    $url_options['language'] = $options['language'];
    $language_code = $options['language']->language;
  }
  else {
    $language_code = NULL;
  }

  $sanitize = !empty($options['sanitize']);

  $replacements = array();

  if ($type == 'line_item' && !empty($data['line_item'])) {
    $line_item = $data['line_item'];

    foreach ($tokens as $name => $original) {
      switch ($name) {
        // Simple key values on the line item.
        case 'line-item-id':
          $replacements[$original] = $line_item->line_item_id;
          break;

        case 'type':
          $replacements[$original] = $sanitize ? check_plain(commerce_line_item_type_get_name($line_item->type)) : commerce_line_item_type_get_name($line_item->type);
          break;

        case 'line-item-label':
          $replacements[$original] = $sanitize ? check_plain($line_item->line_item_label) : $line_item->line_item_label;
          break;

        case 'quantity':
          $replacements[$original] = $sanitize ? check_plain($line_item->quantity) : $line_item->quantity;
          break;

        // Default values for the chained tokens handled below.
        case 'created':
          $replacements[$original] = format_date($line_item->created, 'medium', '', NULL, $language_code);
          break;

        case 'changed':
          $replacements[$original] = format_date($line_item->changed, 'medium', '', NULL, $language_code);
          break;

      }
    }
  }

  return $replacements;
}
