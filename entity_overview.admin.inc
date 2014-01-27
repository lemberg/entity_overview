<?php

/**
 * @file
 * Menu callbacks for entity overview management pages.
 */

/**
 * Menu callback for the entity overview overview page.
 */
function entity_overview_page() {

  $overview = entity_overview_controller(func_get_args());

  return $overview->render();
}
