<?php

function ww_upgrade_1x_to_2x(){
  global $wpdb;

  // create new tables
  ww_post_widgets_table();
  ww_widget_data_table();
  ww_widget_spaces_table();
  ww_widget_space_term_relationships_table();
  ww_default_spaces();
  
  $posts_table = $wpdb->prefix.'posts';
  $postmeta_table = $wpdb->prefix.'postmeta';

  $post_widgets_table  = $wpdb->prefix.'ww_post_widgets';
  $widget_data_table   = $wpdb->prefix.'ww_widget_data';
  $widget_spaces_table = $wpdb->prefix.'ww_widget_spaces';
  $space_terms_table   = $wpdb->prefix.'ww_space_term_relationship';


  // get all widgets and convert their meta fields to the new ones
  $sql = 'SELECT ID, post_type FROM '.$posts_table.' WHERE post_type = "widget"';
  $rows = $wpdb->get_results($sql);
  foreach($rows as $row){
    // ww-adv-enabled = [standard][rewrite-output]
    $ww_adv_enabled = $wpdb->get_row('
          SELECT meta_id, post_id, meta_key, meta_value
          FROM '.$postmeta_table.'
          WHERE post_id = '.$row->ID.'
          AND meta_key = "ww-adv-enabled"');

    // ww-wpautop = [standard][filter-content]
    $ww_wpautop = $wpdb->get_row('
          SELECT meta_id, post_id, meta_key, meta_value
          FROM '.$postmeta_table.'
          WHERE post_id = '.$row->ID.'
          AND meta_key = "ww-wpautop"');

    // ww-adv-template = [standard][adv-template]
    $ww_adv_template = $wpdb->get_row('
          SELECT meta_id, post_id, meta_key, meta_value
          FROM '.$postmeta_table.'
          WHERE post_id = '.$row->ID.'
          AND meta_key = "ww-adv-template"');

    // ww-parse = [standard][custom-output]
    $ww_parse = $wpdb->get_row('
          SELECT meta_id, post_id, meta_key, meta_value
          FROM '.$postmeta_table.'
          WHERE post_id = '.$row->ID.'
          AND meta_key = "ww-parse"');

    $data = array();
    $data['rewrite-output'] = $ww_adv_enabled->meta_value;
    $data['filter-content'] = $ww_wpautop->meta_value;
    $data['adv-template'] = $ww_adv_template->meta_value;
    $data['custom-output'] = $ww_parse->meta_value;

    $insert_data = array(
      'post_id' => $row->ID,
      'type' => 'standard',
      'data' => serialize($new_data),
    );

    $wpdb->insert($widget_data_table, $insert_data, array('%d', '%s', '#s'));

    // test for success
    if (1){
      // delete old postmeta data
      $delete = 'DELETE FROM '.$postmeta_table.'
                  WHERE post_id = '.$row->ID.'
                  AND meta_key IN("ww-adv-enabled","ww-wpautop","ww-adv-template","ww-parse")';
      $wpdb->query($delete);
    }
  }

  // get the default widgets and update the space
  if($default_widgets = get_option('ww_default_widgets')) {
    $default_data = array(
      'widgets' => $default_widgets,
    );
    $where = array('id' => 1);
    $wpdb->update( $widget_spaces_table, $default, $where);

    // clean up old data
    delete_option('ww_default_widgets');
  }

  // get the postspage widgets and update the space
  if($postspage_widgets = get_option('ww_postspage_widgets')){
    $postspage_data = array(
      'widgets' => $postspage_widgets,
    );
    $where = array('id' => 2);
    $wpdb->update( $widget_spaces_table, $postspage_data, $where);

    // clean up old data
    delete_option('ww_postspage_widgets');
  }

  /*
   * Widgets on Pages
   */
  $sql = "SELECT meta_id, post_id, meta_key, meta_value
          FROM ".$postmeta_table."
          WHERE meta_key = 'ww_post_widgets'";
  $rows = $wpdb->get_results($sql);
  foreach($rows as $row){
    $insert = array(
      'post_id' => $row->post_id,
      'widgets' => $row->meta_value,
    );
    $wpdb->insert($post_widgets_table, $insert, array('%d','%s'));

    // clean up old data
    $wpdb->query("DELETE FROM ".$postmeta_table."
                  WHERE meta_id = ".$row->meta_id."
                  AND post_id = ".$row->post_id."
                  AND meta_key = 'ww_post_widgets'");
  }

  /*
   * Settings
   */
  $settings = ww_get_settings();
  // make sure we handle default post_types
  if (!is_array($settings['post_types'])){
    $settings['post_types'] = array('page' => 'page', 'post' => 'post');
    ww_settings_save(array('settings' => serialize($settings)));
  }

}