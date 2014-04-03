<?php

/**
 * @file
 * Provides default class for entity overview pages.
 */

/**
 * Default class for entity objects overview table.
 */
class EntityOverviewDefaultUIController {

  const ENTITIES_PER_PAGE = 25;

  /**
   * Entity type
   *
   * @var string
   */
  protected $entityType;

  /**
   * Entity info
   *
   * @var array
   */
  protected $entityInfo;

  /**
   * Primary database key of a given entity type
   *
   * @var integer
   */
  protected $idKey;

  /**
   * Base path for the entity overview page
   *
   * @var string
   */
  protected $path;

  /**
   * EFQ property conditions.
   *
   * @var array
   */
  protected $conditions = array();

  /**
   * EFQ object.
   *
   * @var EntityFieldQuery
   */
  protected $query = NULL;

  public function __construct($entity_type, $entity_info) {
    $this->entityType = $entity_type;
    $this->entityInfo = $entity_info;
    $this->idKey = $this->entityInfo['entity keys']['id'];
    $this->path = $this->entityInfo['overview ui']['path'];
  }

  /**
   * Implements hook_menu().
   */
  public function hook_menu() {
    $items = array();
    $plural_label = isset($this->entityInfo['plural label']) ? $this->entityInfo['plural label'] : $this->entityInfo['label'] . 's';

    $arguments = array($this->entityType, $this->entityInfo);
    $map = explode('/', $this->path);
    foreach ($map as $key => $item) {
      if (strpos($item, '%') !== FALSE) {
        $arguments[] = $key;
      }
    }

    $items[$this->path] = array(
      'title' => $plural_label,
      'page callback' => 'entity_overview_page',
      'page arguments' => $arguments,
      'description' => 'Manage ' . $plural_label . '.',
      'access callback' => 'entity_overview_access',
      'access arguments' => $arguments,
      'file' => 'entity_overview.admin.inc',
    );

    return $items;
  }

  /**
   * Implements hook_permission().
   */
  public function hook_permission() {
    $permissions = array();
    $permissions['administer entity'] = array(
      'title' => t('Administer entities'),
    );
    return $permissions;
  }

  /**
   * Queries the DB and returns entity objects.
   *
   * @return array
   *  Array of queried entities.
   */
  protected function query() {
    $this->buildQuery();

    $entities = $this->query->execute();
    if (!empty($entities[$this->entityType])) {
      $ids = array_keys($entities[$this->entityType]);

      return entity_load($this->entityType, $ids);
    }

    return array();
  }

  /**
   * Builds Entity field query.
   */
  protected function buildQuery() {
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', $this->entityType)
            ->pager(static::ENTITIES_PER_PAGE);

    $this->applyFilter();

    foreach ($this->conditions as $column => $value) {
      if (is_array($value)) {
        $value += array(
          'type' => 'property',
          'column' => $column,
          'operator' => '=',
        );
        switch ($value['type']) {
          case 'property':
            $query->propertyCondition($value['column'], $value['value'], $value['operator']);
            break;
          case 'field':
            $query->fieldCondition($value['field'], $value['column'], $value['value'], $value['operator']);
            break;
        }
      }
      else {
        $query->propertyCondition($column, $value);
      }
    }
    // Store the query.
    $this->query = $query;
  }

  /**
   * Returns overview table renderable array.
   *
   * @return array
   *  Table definition array, ready for rendering.
   */
  protected function getOverviewTable() {

    $header = $this->getOverviewTableHeader();

    $rows = array();
    $entities = $this->query();
    foreach ($entities as $entity) {
      $rows[] = $this->getOverviewTableRow($entity);
    }

    $output = array(
      '#theme' => 'table',
      '#attributes' => array(
        'class' => array('entity-overview'),
      ),
      '#rows' => $rows,
      '#header' => $header,
      '#empty' => t('Nothing found.'),
    );

    return $output;
  }

  /**
   * Returns table header for a particular entity type.
   */
  protected function getOverviewTableHeader() {
    return array(
      t('ID'),
      t('Title'),
      t('Operations'),
    );
  }

  /**
   * Returns single table row for a given entity.
   */
  protected function getOverviewTableRow($entity) {
    return array(
      $entity->{$this->idKey},
      entity_label($this->entityType, $entity),
      array('data' => $this->getOperations($entity)),
    );
  }

  /**
   * Returns operations links for a given entity.
   */
  protected function getOperations($entity) {
    $base = array(
      'query' => drupal_get_destination(),
    );
    // @todo: Add access checks.
    $links = array();
    $links[] = array(
      'title' => t('edit'),
      'href' => "$this->entityType/{$entity->{$this->idKey}}/edit",
            ) + $base;
    $links[] = array(
      'title' => t('delete'),
      'href' => "$this->entityType/{$entity->{$this->idKey}}/delete",
            ) + $base;

    return array(
      '#theme' => 'links',
      '#links' => $links,
      '#attributes' => array(
        'class' => array('links', 'inline'),
      ),
    );
  }

  /**
   * Builds filter form.
   *
   * @return array
   *  Filter form renderable array.
   */
  protected function getFilterForm() {
    return filter_form_build('entity_overview', $this->entityType);
  }

  /**
   * Whether entity over view controller in question exposes filter form.
   *
   * @return boolean
   *  Flag indicating filter form existence.
   */
  protected function hasFilterForm() {
    return FALSE;
  }

  /**
   * Returns filter form definition array.
   *
   * @return array
   *  Filter form renderable array.
   */
  public function filterForm(&$form, &$form_state) {
    return array();
  }

  /**
   * Returns filter form values.
   *
   * @return array
   *  Array of
   */
  protected function getFilterValues() {
    return filter_form_get_values($this->entityType);
  }

  /**
   * Applies filter values.
   */
  protected function applyFilter(EntityFieldQuery &$query) {
    $values = $this->getFilterValues();

    foreach ($values as $key => $value) {
      $this->conditions[$key] = $value;
    }
  }

  /**
   * Checks whether current user have access to the overview page in question.
   */
  public function access() {
    return TRUE;
  }

  /**
   * Returns entity objects overview table.
   *
   * @return array
   *  Renderable array of entity objects overview table.
   */
  public function render() {
    $output = array();

    if ($this->hasFilterForm()) {
      $output['filter_form'] = $this->getFilterForm();
    }
    $output['overview_table'] = $this->getOverviewTable();
    $output['pager'] = array(
      '#theme' => 'pager',
    );

    return $output;
  }

}
