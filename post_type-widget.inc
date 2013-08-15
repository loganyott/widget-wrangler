<?php
// Initiate the plugin
add_action( 'init', 'Widget_Wrangler_Init');
/*
 * Function for initializing the plugin
 */
function Widget_Wrangler_Init() {
  global $ww;
  $ww = new Widget_Wrangler();
}
/*
 * Widget Wrangler custom_post_type class for widget post type
 */
class Widget_Wrangler {
  var $data = array();
  var $type;
  var $post_id;
  
  /*
   * Constructor, build the new post type
   */
  function Widget_Wrangler()
  {
    // Editing specific widget
    if (isset($_GET['post'])){
      $this->post_id = $_GET['post'];
    }
    else if (isset($_POST['post_ID'])){
      $this->post_id = $_POST['post_ID'];
    }
    
    // this widget has been saved before
    if(isset($this->post_id)){
      $widget = ww_get_widget_data($this->post_id);
      $this->type = $widget->type;
      $this->data = unserialize($widget->data);
      $supports = array(
        'title',
        'excerpt'
      );
      if($this->type == 'standard'){
        $supports[] = 'editor';
      }
    }
    // this is a new widget
    else
    {
      $supports = array(
        'title',
        'excerpt',
        'editor',
        /*,'custom-fields'*/
      );
    }
    
    // get ww settings
    $settings = ww_get_settings();
    // allow for custom capability type
    $capability_type = ($settings['capabilities'] == "advanced" && isset($settings['advanced'])) ? $settings['advanced'] : "post";
    $labels = array(
      'name' => _x('Widget Wrangler', 'post type general name'),
      'all_items' => __('All Widgets'),
      'singular_name' => _x('Widget', 'post type singular name'),
      'add_new' => _x('Add New', 'widget'),
      'add_new_item' => __('Add New Widget'),
      'edit_item' => __('Edit Widget'),
      'new_item' => __('New Widget'),
      'view_item' => __('View Widget'),
      'search_items' => __('Search Widgets'),
      'menu_icon' => WW_PLUGIN_DIR.'/icon-wrangler.png',
      'not_found' =>  __('No widgets found'),
      'not_found_in_trash' => __('No widgets found in Trash'), 
      'parent_item_colon' => '',
    );
    // Register custom post types
    register_post_type('widget', array(
      'labels' =>$labels,
      'public' => true,
      'exclude_from_search' => true, // @TODO: setting ?
      'show_in_menu' => true,
      'show_ui' => true, // UI in admin panel
      '_builtin' => false, // It's a custom post type, not built in
      '_edit_link' => 'post.php?post=%d',
      'capability_type' => $capability_type,
      'hierarchical' => false,
      'rewrite' => array("slug" => "widget"), // Permalinks
      'query_var' => "widget", // This goes to the WP_Query schema
      'supports' => $supports,
      'menu_icon' => WW_PLUGIN_URL.'/images/wrangler_icon.png'
    ));
   
    add_filter("manage_edit-widget_columns", array(&$this, "edit_columns"));
    add_action("manage_posts_custom_column", array(&$this, "custom_columns"));
   
    // Admin interface init
    add_action("admin_init", array(&$this, "admin_init"));
    //add_action("template_redirect", array(&$this, 'template_redirect'));
    
    // Insert post hook
    add_action("wp_insert_post", array(&$this, "wp_insert_post"), 10, 2);
  }
  /*
   * Custom columns for the main Widgets management page
   */  
  function edit_columns($columns)
  {
    $columns = array(
      "cb" => "<input type=\"checkbox\" />",
      "title" => "Widget Title",
      "ww_type" => "Type",
      "ww_description" => "Description",
      "ww_rewrite_output" => "Rewrite Output",
    );
   
    return $columns;
  }
  /*
   * Handler for custom columns
   */
  function custom_columns($column)
  {
    global $post;
    $widget = ww_get_widget_data($post->ID);
    $data = unserialize($widget->data);
    
    switch ($column)
    {
      case "ww_type":
        print $widget->type;
        break;
      case "ww_description":
        the_excerpt();
        break;
      case "ww_rewrite_output":
        print $data['rewrite-output'];
        break;
    }
  }
  
  /*
   * When a post is inserted or updated
   */ 
  function wp_insert_post($post_id, $post = null)
  {
    if ($post->post_type == "widget")
    {
      global $wpdb;
      $type = (isset($_POST['ww-type'])) ? $_POST['ww-type'] : "";
      
      // problems with php and html adding slashes
      if(isset($_POST['ww-data']['standard']['custom-output'])){
        $_POST['ww-data']['standard']['custom-output'] = trim(stripcslashes($_POST['ww-data']['standard']['custom-output']));
      }
      if(isset($_POST['ww-data']['logic']['php'])){
        $_POST['ww-data']['logic']['php'] = trim(stripcslashes($_POST['ww-data']['logic']['php']));
      }
      
      // clones
      if($type == 'clone' && $_POST['ww-data']['clone']['clone-class']){
        global $wp_widget_factory;
        $clone_class = $_POST['ww-data']['clone']['clone-class'];
        $option_name = str_replace("_", "-", $wp_widget_factory->widgets[$clone_class]->option_name);
        $instance = array();
        
        // loop through instance values and create an instance array
        foreach($_POST[$option_name] as $i => $settings)
        {
          foreach($settings as $key => $value)
          {
            $instance[$key] = $value;
          }
        }
        
        $_POST['ww-data']['clone']['clone-instance'] = serialize($instance);
      }
      
      // prep the data and logic for save
      $widget_data = array();
      if (isset($_POST['ww-data'])) {
        $widget_data = $_POST['ww-data'][$type];
        $widget_data['logic'] = $_POST['ww-data']['logic'];
      }
      
      // unset the php value if empty
      if(isset($widget_data['logic']) && $widget_data['logic']['php'] == ''){
        unset($widget_data['logic']['php']);
      }
      $data = serialize($widget_data);

      // see if this post has data
      $exists = $wpdb->get_results("SELECT type FROM ".$wpdb->prefix."ww_widget_data WHERE post_id = ".$post_id);
      
      // update if exists
      if(count($exists) > 0){
        $wpdb->update($wpdb->prefix.'ww_widget_data', array('type' => $type, 'data' => $data), array('post_id' => $post_id));
      }
      // insert if not
      else {
        $wpdb->insert($wpdb->prefix.'ww_widget_data', array('post_id' => $post_id, 'type' => $type, 'data' => $data));
      }
    }
  }
  /*
   * Add meta box to widget posts
   */
  // Custom meta boxes for the edit widget screen
  function admin_init() 
  {
    // add js
    wp_enqueue_script('ww-widget-js',
                    plugins_url('/js/widget.js', WW_PLUGIN_DIR.'/js' ),
                    array('jquery-ui-core', 'jquery-ui-tabs'),
                    false,
                    true);

    // add meta boxes
    if (isset($_GET['post'])){
      $post_id = $_GET['post'];
    }
    else if (isset($_POST['post_ID'])){
      $post_id = $_POST['post_ID'];
    }
    
    if (isset($post_id)){
      $widget = ww_get_widget_data($post_id);
  
      if(isset($widget->type) && $widget->type == "clone")
      {
        // Clone Class
        add_meta_box("ww-clone-class", "Clone Widget", array(&$this, "meta_clone_widget"), "widget", "side", "high");  
        // Clone Instance
        add_meta_box("ww-clone-instance", "Clone Instance", array(&$this, "meta_clone_instance"), "widget", "normal", "high");  
      }
      else if (!isset($widget->type) || $widget->type == 'standard')
      {
        // Rewrite output box
        add_meta_box("ww-parse", "Options", array(&$this, "meta_parse"), "widget", "normal", "high");
      }
    }
    
    // Widget type
    add_meta_box("ww-type", "Widget Details", array(&$this, "meta_widget_details"), "widget", "side", "high");

    // Widget Preview
    add_meta_box("ww-widget-preview", "Widget Preview", array(&$this, "meta_widget_preview"), "widget", "side", "default");
    
    // Widget Display Logic
    add_meta_box("ww-display-logic", "Display Logic", array(&$this, "meta_display_logic"), "widget", "normal", "default");
  }
  
  // Widget Type
  function meta_widget_details()
  {
    $widget_types = ww_all_widget_types();
    
    // use standard as a sdefault for new widgets
    if(!isset($this->type)){
      $this->type = 'standard';
    } 
    // output field
    ?>
      <div>
        <?php
          if($this->post_id)
          { ?>
            <div class="ww-widget-postid">
              Post ID<br/>
              <span><?php print $this->post_id;?></span>
            </div>
            <?php
          }
        ?>
        <div>
          <h4>Widget Type:</h4>
          <?php print $widget_types[$this->type]; ?>
        </div>
        <?php
          // widget shortcode
          if($this->post_id)
          { ?>
            <div>
              <h4>Widget Shortcode:</h4>
              [ww_widget  id=<?php print $this->post_id; ?>]
            </div>
            <?php
          }
        ?>
        <input type="hidden" name="ww-type" value="<?php print $this->type; ?>" />
        
        <div class="ww-clear-gone"><!-- ie hack -->&nbps;</div>
      </div>
    <?php
  }
  
  // Display logic
  function meta_display_logic()
  {
    $data = $this->data;
    ?>
    <p><em>
      The Widget's display logic lets you choose which Wordpress pages the widget is allowed to be displayed on.
      <br/>
      If nothing is selected, the widget will display in all cases.
      </em>
    </p>
    <div id="logic-tabs">
      <ul id="logic-tabs-list">
        <li><a href="#tab-post-types"><span>Post Types</span></a></li>
        <!--<li><a href="#tab-conditional-tags"><span>Conditional Tags</span></a></li>-->
        <li><a href="#tab-php"><span>Custom PHP</span></a></li>
      </ul>
      <?php
        /*
         * Post types
         */ 
        // Get all extra post types
        $post_types = get_post_types(array('public' => true, '_builtin' => false),'names','and');
        $post_types['post'] = 'post';
        $post_types['page'] = 'page';
        unset($post_types['widget']);
        ksort($post_types);    
      ?>
        <div id="tab-post-types">
          <p>
            <strong>Select which post types this widget should display on.</strong>
          </p>
          <div class="space-checkboxes">
            <?php  
              // loop through post types
              foreach ($post_types as $post_type )
              {
                $post_type_checked = (isset($data['logic']['post_types'][$post_type])) ? 'checked="checked"' : '';
                ?>
                <label class="ww-space-checkbox"><input type="checkbox" name="ww-data[logic][post_types][<?php print $post_type; ?>]" value="<?php print $post_type; ?>" <?php print $post_type_checked; ?> /><?php print ucfirst($post_type); ?> </label>
                <?php
              }
            ?>
          </div>
        </div>
      <?php
        /*
         * Custom php
         */
        $data_logic_php = (isset($data['logic']['php'])) ? htmlentities($data['logic']['php']) : "";
      ?>
        <div id="tab-php">
          <div class="logic-php-wrapper">
            <p>
              <strong>
                Use custom php to determine if the widget should be displayed.
                <ul>
                  <li>Must return true to display the widget.</li>
                  <li>Do not! use open and close php tags. (eg &lt;php , ?&gt;)</li>
                </ul>
              </strong>
            </p>
            <textarea name="ww-data[logic][php]" cols="40" rows="16" style="width: 100%;"><?php print $data_logic_php; ?></textarea>
          </div>
        </div>
        <div class="ww-clear-gone"><!-- ie hack -->&nbps;</div>
    </div>
    <?php
  }
  
  // Clone Widget
  function meta_clone_widget()
  {
    global $wp_widget_factory;
    $data = $this->data;
    $clone_class = $data['clone-class'];
    
    // hidden value if set
    if($clone_class) {
      ?>
        <h4><?php print $wp_widget_factory->widgets[$clone_class]->name; ?></h4>
        <input type="hidden" name="ww-data[clone][clone-class]" value="<?php print $clone_class; ?>" />
      <?php
    }
    // select box if not set
    else {
      ?>
        <select name="ww-data[clone][clone-class]">
          <?php
            foreach ($wp_widget_factory->widgets as $class_name => $widget)
            {
              $this_widget_selected = ($clone_class == $class_name) ? 'selected="selected"':'';
              ?>
              <option value="<?php print $class_name; ?>" <?php print $this_widget_selected; ?>><?php print $widget->name; ?></option>
              <?php
            }
          ?>
        </select>
      <?php
    }
  }
  
  // Clone Instance
  function meta_clone_instance()
  {
    // get widget factory and post data
    global $wp_widget_factory;

    $data = $this->data;
    $clone_class = $data['clone-class'];
    
    if($clone_class)
    {
      $instance = unserialize($data['clone-instance']);

      // create instance form
      ob_start();
        eval('$w = new '.$clone_class.'(); $w->form($instance);');
        $instance_form = ob_get_clean();
        
      $hide_title_checked = (isset($data['hide_title'])) ? 'checked="checked"' : '';
      ?>
        <label>
          <input type="checkbox" name="ww-data[clone][hide_title]" <?php print $hide_title_checked; ?> /> Hide the Widget's title on display
        </label>
        <div class="ww-clone-form">
          <?php print $instance_form; ?>
        </div>
        <input type="hidden" name="ww-data[clone][clone-instance]" value="Placeholder" />
      <?php
    }
  }
    
  // Admin preview box
  function meta_widget_preview()
  {
    if ($this->post_id)
    {
      // query widgets break edit page
      if(isset($this->data['clone-class']) && $this->data['clone-class'] == "Query_Wrangler_Widget") {
        print "Query Widgets cannot be previewed";
      }
      else {
        $widget = ww_get_single_widget($this->post_id);
        ?>
          <div id="ww-preview">
            <p><em>This preview does not include your theme's CSS stylesheet.</em></p>
            <?php print ww_theme_single_widget($widget); ?>
          </div>
        <?php
      }   
    }
  }
  
  // Admin post meta contents
  function meta_parse()
  {
    $data = $this->data;
    
    // default to checked upon creation
    $rewrite_output_checked = (isset($data['rewrite-output'])) ? 'checked="checked"' : '';
    $adv_template_checked   = (isset($data['adv-template'])) ? 'checked="checked"' : '';
    $filter_content_checked = (isset($data['filter-content']) || (($_GET['action'] == null) && ($_GET['post_type'] == 'widget'))) ? 'checked="checked"' : '';
    $data_custom_output     = (isset($data['custom-output'])) ? htmlentities($data['custom-output']) : "";
    ?>
      <div id="ww-template">
        <div>
          <label><input type="checkbox" name="ww-data[standard][filter-content]" <?php print $filter_content_checked; ?> /> Apply Content filters to this Widget's Content.</label>
        </div>
        <div>
          <h4>Rewrite Output</h4>
          <div class="ww-options-group">
            <div class="ww-options-group-title">
              <label><input id="ww-adv-parse-toggle" type="checkbox" name="ww-data[standard][rewrite-output]" <?php print $rewrite_output_checked; ?> /> Rewrite the output of this widget</label>
            </div>
            <div class="ww-options-group-content ww-options-hidden">
              <div id="ww-advanced-template">
                <label><input id="ww-adv-template-toggle" type="checkbox" name="ww-data[standard][adv-template]" <?php print $adv_template_checked; ?> /> Template the Advanced Parsing Area</label>
              </div>
              <textarea name="ww-data[standard][custom-output]" cols="40" rows="16" style="width: 100%;"><?php print $data_custom_output; ?></textarea>
              <div class="adv-parse-description">
                <h5>In the Advanced Parsing area you can:</h5>
                <ul>
                  <li>Use PHP ( &lt;?php and ?&gt; are required )</li>
                  <li>Use {{title}} or $widget->post_title to insert the widget's title</li>
                  <li>Use {{content}} or $widget->post_content to insert the widget's content</li>
                  <li>Access the $widget object for more widget data (see provided template for examples)</li>
                  <li>Access the $post object for data concerning the page being displayed (see provided template for examples)</li>
                </ul>
                <h5>Templating Advanced Parsed Widgets</h5>
                <ul>
                  <li>To template an advanced parsed widget you must return an associative array with a title and content string.</li>
                  <li>Example: &lt;?php return array("title" => "The Widget's Title", "content" => "The Widget's Content"); ?&gt;</li>
                </ul>
              </div>
            </div>
          </div> <!-- ww-options-group -->
        </div>
      </div>
    <?php
  }
}
// end widget class
 