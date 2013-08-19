<?php
/*
 * Template Wrangler 1.2
 *
 * Functions
 * theme() - template some arguments
 *
 */

/*
 * Main Templating function
 *
 * @param string $template_name The array key given when defining a new template
 * @param array $arguments Array of variables to pass using key -> value pairs. In the template file the key will be the variable name, eg $key
 *
 * @return string Template HTML
 */
function theme($template_name = '', $arguments = array())
{
	
  if(is_null($arguments)){
    $arguments = array();
  }

  // get all the hooks
  $templates = tw_templates();

  // make sure given templates key exists
  if(is_array($templates[$template_name]))
  {
    // template we're using
    $template = $templates[$template_name];
    $template['name'] = $template_name;
		
    // if not arguments array given, make empty array for future actions
    if(!isset($template['arguments']) || !is_array($template['arguments'])){
      $template['arguments'] = array();
    }

    // merge passed arguments into array overriding defaults
    // update the template arguments for preprocess functions
    $template['arguments'] = array_merge($template['arguments'], $arguments);

    // wp style hook for preprocess
    do_action_ref_array('tw_pre_process_template', array(&$template));

		// theme functions return now if found
    if ($theme = tw_process_template($template)){
			return $theme;
		}
    tw_find_template($template);

    // for other means of processing or information
    if (isset($template['arguments']['tw_action']) && $template['arguments']['tw_action'] == 'find_only'){
      return $template;
    }

    // proceed if file found
    if($template['found_path'] != '')
    {
      // allow late modifications
      do_action_ref_array('tw_pre_render_template', array(&$template));

      // start output buffer
      ob_start();

        // turn arguments into variables
        extract($template['arguments']);

        // include the file
        include $template['found_path'];

      // return output buffer
      $theme = ob_get_clean();
      return $theme;
    }
  }
}

/*
 * Look for non-wordpress preprocess functions
 */
function tw_process_template(&$template){
  // get theme folder name
  $theme_folder = explode("/", TEMPLATEPATH);
  $theme_folder = array_pop($theme_folder);

  // if no default_path, set to function execution path
  if(!isset($template['default_path']))
  {
    // get function caller backtrace
    $bt = debug_backtrace();
    $caller = array_shift($bt);

    // set the function caller's file
    // @TODO: instead, find a way to set the default path to where the hook was set
    $template['default_path'] = dirname($caller['file']);
  }

  // if no templates were given, look for theme functions
  if(!isset($template['files']))
  {
    // look for theme folder name name based function
    if(function_exists($theme_folder.'_'.$template['name'])){
      // function found, execute it and return the results
      return call_user_func_array($theme_folder.'_'.$template['name'], $template['arguments']);
    }
    // see if a function using the template name exists
    else if(function_exists('theme_'.$template['name'])){
      // function found, execute it and return the results
      return call_user_func_array('theme_'.$template['name'], $template['arguments']);
    }
  }
  // else if given templates is not an array, make it one
  else if(!is_array($template['files']))
  {
    $template['files'] = array($template['files']);
  }
}

/*
 * Gather suggestions and look for existing file
 */
function tw_find_template(&$template){
	
  // loop through and find suggested templates
  $found_path = '';
  foreach($template['files'] as $suggestion)
  {
    // look for an argument in template suggestions
    if (preg_match('/\[.*\]/',$suggestion))
    {
      // we have arguments, lets build the possibilities
      $search = array();
      $replace = array();
      foreach($template['arguments'] as $key => $value)
      {
        // only apply to strings and numerics
        if(is_string($template['arguments'][$key]) || is_numeric($template['arguments'][$key])){
          $search[] = '['.$key.']';
          $replace[] = $value;
        }
      }

      // do the replacement
      $suggestion = str_replace($search, $replace, $suggestion);
    }
    $template['suggestions'][] = $suggestion;
  }

  // loop through suggestions and find existing template in theme path first
  foreach($template['suggestions'] as $suggestion){
    // easier to read this way
    $theme_path = locate_template($suggestion, false, false);

    // look in the theme folder
    if(file_exists($theme_path)){
      $template['found_path'] = $theme_path;
      $template['found_suggestion'] = $suggestion;
      break;
    }
  }

  if (!isset($template['found_path'])){
    // loop through suggestions and find existing template in default path
    foreach($template['suggestions'] as $suggestion){
      $default_path = rtrim($template['default_path']).'/'.$suggestion;

      // look for file in default path
      if(file_exists($default_path)){
        $template['found_path'] = $default_path;
        $template['found_suggestion'] = $suggestion;
        break;
      }
    }
  }
}

/*
 * Hook for adding templates to the system
 *
 * @return array All templates
 */
function tw_templates(){
  $templates = apply_filters('tw_templates', array());
  return $templates;
}

/*
 * EXAMPLES
 *
// how to include template-wrangler in a plugin
// include Template Wrangler
if(!function_exists('theme')){
  include_once QW_PLUGIN_DIR.'/template-wrangler.inc';
}

// example template hook
function example_template($templates)
{
  // template applied by files
  $templates['example_template'] = array(
    // string of single template suggestion
    // or array of multiple template suggestions
    'files' => array(
        // use argument keys as replacable patterns with the argument's value
        // order is important as the order here is the order inwhich templates are searched for
        'mytemplate-[arg1]-[arg2].php',
        'mytemplate-[arg1].php',
        // a default fall back template name
        'mytemplate.php'
    ),

    // location of the default template if nothing is found
    'default_path' => dirname(__FILE__),

    // optional arguments to be passed to the themeing function
    'arguments' => array(
        // must be key => value pairs
        'arg1' => 'arg1_default_value',
        'arg2' => 'arg2_default_value',
    ),
  );

  // theme function example
  // since no template files are specified, it will attempt to run a function named theme_link()
  // it will look first for a funtion name THEME_FOLDER_NAME_link()
  // example: if your active theme is in a folder name my_theme, it will look for my_theme_link()
  $templates['link'] = array(
    'default_path' => dirname(__FILE__),
    'arguments' => array(
      'href' => 'http://google.com',
      'text' => 'your link text',
    )
  );

  return $templates;
}
// hook your function into the wordpress filter
add_filter('tw_templates', 'example_template');

// example preprocess function
function example_template_preprocess($template)
{
  // change argument values
  $template['arguments']['arg2'] = '1';
  // add a new argument
  $template['arguments']['new_arg'] = 'brand new argument';

  // add a new template suggestions as the first suggestion
  array_unshift($template['files'], 'mytemplate-new-first-suggestion.php');
  // add a new template suggestion as the last suggestion
  $template['files'][] = 'mytemplate-new-final-suggestion.php';

  // return the modified template
  return $template;
}

// example theme function
function theme_link($href, $text){
  return "<a href='$href'>$text</a>";
}
*/