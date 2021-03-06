<?php
// $Id$

/**
 * @file
 * Callback functions for the Payment module's checkout panes.
 */


/**
 * Payment pane: form callback.
 */
function commerce_payment_pane_checkout_form($form, &$form_state, $checkout_pane, $order) {
  $pane_form = array();

  // Invoke the payment methods event that will populate the order with
  // an array of method IDs for available payment methods.
  $order->payment_methods = array();
  rules_invoke_event('commerce_payment_methods', $order);

  // Generate an array of payment method options for the checkout form.
  $options = array();

  foreach ((array) $order->payment_methods as $method_id) {
    if ($payment_method = commerce_payment_method_load($method_id)) {
      $options[$method_id] = $payment_method->title;
    }
  }

  // If no payment methods were found, return the empty form.
  if (empty($options)) {
    return $pane_form;
  }

  // Store the payment methods in the form for validation purposes.
  $pane_form['payment_methods'] = array(
    '#type' => 'value',
    '#value' => $order->payment_methods,
  );

  // If at least one payment option is available...
  if (!empty($options)) {
    // Add a radio select widget to specify the payment method.
    $pane_form['payment_method'] = array(
      '#type' => 'radios',
      '#options' => $options,
    );

    // Find the default payment method using either the preselected value stored
    // in the order / checkout pane or the first available method.
    $pane_values = !empty($form_state['values']) ? $form_state['values'][$checkout_pane->pane_id] : array();

    if (isset($order->data['payment_method']) && isset($options[$order->data['payment_method']])) {
      $default_value = $order->data['payment_method'];
    }
    elseif (isset($pane_values['payment_method']) && isset($options[$pane_values['payment_method']])) {
      $default_value = $pane_values['payment_method'];
    }
    else {
      $default_value = array_shift(array_keys($options));
    }

    // Set the default value for the payment method radios.
    $pane_form['payment_method']['#default_value'] = $default_value;

    // Add the payment method specific form elements.
    $payment_method = commerce_payment_method_load($pane_form['payment_method']['#default_value']);

    if (function_exists($callback = $payment_method->callbacks['submit_form'])) {
      $pane_form['payment_details'] = $callback($payment_method, $pane_values, $checkout_pane, $order);
    }
    else {
      $pane_form['payment_details'] = array();
    }
  }

  return $pane_form;
}

/**
 * Payment pane: validation callback.
 */
function commerce_payment_pane_checkout_form_validate($form, &$form_state, $checkout_pane, $order) {
  $pane_form = $form[$checkout_pane->pane_id];
  $pane_values = $form_state['values'][$checkout_pane->pane_id];

  // If the selected payment method was changed...
  if ($pane_values['payment_method'] != $pane_form['payment_method']['#default_value']) {
    // And the newly selected method has a valid form callback...
    if ($payment_method = commerce_payment_method_load($pane_values['payment_method'])) {
      if (function_exists($payment_method->callbacks['submit_form'])) {
        // Fail validation so the form is rebuilt to include the payment method
        // specific form elements.
        return FALSE;
      }
    }
  }

  // Delegate validation to the payment method callback.
  $payment_method = commerce_payment_method_load($pane_values['payment_method']);

  if (function_exists($callback = $payment_method->callbacks['submit_form_validate'])) {
    $result = $callback($payment_method, $pane_form['payment_details'], $pane_values['payment_details'], $order);

    // To prevent payment method validation routines from having to return TRUE
    // explicitly, only return FALSE if it was specifically returned.  Otherwise
    // default to TRUE.
    return $result === FALSE ? FALSE : TRUE;
  }

  // Nothing to validate.
  return TRUE;
}

/**
 * Payment pane: submit callback.
 */
function commerce_payment_pane_checkout_form_submit($form, &$form_state, $checkout_pane, $order) {
  $pane_form = $form[$checkout_pane->pane_id];
  $pane_values = $form_state['values'][$checkout_pane->pane_id];

  $order->data['payment_method'] = $pane_values['payment_method'];

  // Delegate submit to the payment method callback.
  $payment_method = commerce_payment_method_load($pane_values['payment_method']);

  if (function_exists($callback = $payment_method->callbacks['submit_form_submit'])) {
    return $callback($payment_method, $pane_form['payment_details'], $pane_values['payment_details'], $order);
  }
}

/**
 * Payment redirect pane: form callback.
 */
function commerce_payment_redirect_pane_checkout_form(&$form_state, $pane_values, $checkout_pane, $order) {
  $form = array();

  // Delegate to the payment method if it can be loaded.
  if (!$payment_method = commerce_payment_method_load($order->data['payment_method'])) {
    return $form;
  }

  if (function_exists($callback = $payment_method->callbacks['redirect_form'])) {
    $form = $callback($payment_method, $pane_values, $checkout_pane, $order);
  }

  return $form;
}

/**
 * Payment redirect pane: validate callback.
 */
function commerce_payment_redirect_pane_checkout_form_validate($pane_form, &$form_state, $pane_values, $checkout_pane, $order) {
  // Delegate to the payment method.
  $current_payment_method = $order->payment_methods[$order->payment_method];
  $payment_method = commerce_payment_method_load($current_payment_method['method_id']);

  if (function_exists($callback = $payment_method->callbacks['redirect_form_validate'])) {
    return $callback($current_payment_method, $pane_values, $checkout_pane, $order);
  }
  else {
    return TRUE;
  }
}

/**
 * Payment redirect pane: submit callback.
 */
function commerce_payment_redirect_pane_checkout_form_submit($pane_values, $checkout_pane, $order) {
  // Simply pass the control to the payment method.
  $current_payment_method = $order->payment_methods[$order->payment_method];
  $payment_method = commerce_payment_method_load($current_payment_method['method_id']);

  if (function_exists($callback = $payment_method->callbacks['redirect_form_submit'])) {
    $callback($current_payment_method, $pane_values, $checkout_pane, $order);
  }
  else {
    return TRUE;
  }
}

/**
 * Utility function: return a payment redirect page for POST.
 *
 * @param $action
 *   The destination URL the values should be posted to.
 * @param $values
 *   An associative array of values that will be posted to the destination URL.
 * @return
 *   A renderable array.
 */
function commerce_payment_post_redirect_form($action, array $values = array()) {
  $form = array(
    '#type' => 'form',
    '#action' => $action,
    '#method' => 'POST',
    '#id' => '',
    '#attributes' => array(),
  );
  foreach ($values as $key => $value) {
    $form[$value] = array(
      '#type' => 'hidden',
      '#name' => $key,
      '#value' => $value,
      '#id' => '',
      '#attributes' => array(),
    );
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#id' => '',
    '#value' => t('Process to payment'),
  );

  return array(
    'form' => array(
      '#type' => 'markup',
      '#markup' => drupal_render($form),
    ),
  );
}
