// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Set to true when the Document is loaded IFF "test=true" is in the query
// string.
var isTest = false;

// Set to true when loading a "Release" NaCl module, false when loading a
// "Debug" NaCl module.
var isRelease = true;

// Javascript module pattern:
//   see http://en.wikipedia.org/wiki/Unobtrusive_JavaScript#Namespaces
// In essence, we define an anonymous function which is immediately called and
// returns a new object. The new object contains only the exported definitions;
// all other definitions in the anonymous function are inaccessible to external
// code.
var common = (function() {

  function isHostToolchain(tool) {
    return tool == 'win' || tool == 'linux' || tool == 'mac';
  }

  /**
   * Check if the browser supports NaCl plugins.
   *
   * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
   * @return {bool} True if the browser supports the type of NaCl plugin
   * produced by the given toolchain.
   */
  function browserSupportsNaCl(tool) {
    // Assume host toolchains always work with the given browser.
    // The below mime-type checking might not work with
    // --register-pepper-plugins.
    var decoder_mimetype = settings.decoder.mimetype;
    var renderer_mimetype = settings.renderer.mimetype;
    return navigator.mimeTypes[decoder_mimetype] !== undefined && navigator.mimeTypes[renderer_mimetype] !== undefined;
  }
  
  /**
   * Create the Native Client <embed> element as a child of the DOM element
   * named "listener".
   *
   * @param {string} name The name of the example.
   * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
   * @param {string} path Directory name where .nmf file can be found.
   * @param {number} width The width to create the plugin.
   * @param {number} height The height to create the plugin.
   * @param {Object} attrs Dictionary of attributes to set on the module.
   */
  function createNaClModule(name, tool, path, width, height, attrs) {
    var moduleEl = document.createElement('embed');
    moduleEl.setAttribute('name', 'nacl_module');
    moduleEl.setAttribute('id', 'nacl_module');
    moduleEl.setAttribute('width', width);
    moduleEl.setAttribute('height', height);
    moduleEl.setAttribute('src', path + '/' + name + '.nmf');

    var mimetype = settings.mimetype;
    moduleEl.setAttribute('type', mimetype);

    // The <EMBED> element is wrapped inside a <DIV>, which has both a 'load'
    // and a 'message' event listener attached.  This wrapping method is used
    // instead of attaching the event listeners directly to the <EMBED> element
    // to ensure that the listeners are active before the NaCl module 'load'
    // event fires.
    var listenerDiv = document.getElementById('listener');
    listenerDiv.appendChild(moduleEl);

    // Request the offsetTop property to force a relayout. As of Apr 10, 2014
    // this is needed if the module is being loaded on a Chrome App's
    // background page (see crbug.com/350445).
    moduleEl.offsetTop;

    // Host plugins don't send a moduleDidLoad message. We'll fake it here.
    var isHost = isHostToolchain('win');
    if (isHost) {
      window.setTimeout(function() {
        moduleEl.readyState = 1;
        moduleEl.dispatchEvent(new CustomEvent('loadstart'));
        moduleEl.readyState = 4;
        moduleEl.dispatchEvent(new CustomEvent('load'));
        moduleEl.dispatchEvent(new CustomEvent('loadend'));
      }, 100);  // 100 ms
    }
  }
  
  /**
   * Create the Native Client <embed> element as a child of the DOM element
   * named "listener".
   *
   * @param {string} name The name of the example.
   * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
   * @param {string} path Directory name where .nmf file can be found.
   * @param {number} width The width to create the plugin.
   * @param {number} height The height to create the plugin.
   * @param {Object} attrs Dictionary of attributes to set on the module.
   */
  function createDecoder() {
    var moduleEl = document.createElement('embed');
    moduleEl.setAttribute('id', 'decoder');
    moduleEl.setAttribute('width', 0);
    moduleEl.setAttribute('height', 0);
    moduleEl.setAttribute('plugin_type', 'decoder');
    moduleEl.setAttribute('docsrc', settings.document.path);
    if (settings.type == 'pnacl') {
      moduleEl.setAttribute('src', settings.decoder.nmf);
    }

    var mimetype = settings.decoder.mimetype;
    moduleEl.setAttribute('type', mimetype);

    // The <EMBED> element is wrapped inside a <DIV>, which has both a 'load'
    // and a 'message' event listener attached.  This wrapping method is used
    // instead of attaching the event listeners directly to the <EMBED> element
    // to ensure that the listeners are active before the NaCl module 'load'
    // event fires.
    var listenerDiv = document.getElementById('listener');
    listenerDiv.appendChild(moduleEl);

    // Request the offsetTop property to force a relayout. As of Apr 10, 2014
    // this is needed if the module is being loaded on a Chrome App's
    // background page (see crbug.com/350445).
    moduleEl.offsetTop;

    // Host plugins don't send a moduleDidLoad message. We'll fake it here.
    var isHost = isHostToolchain('win');
    if (isHost) {
      window.setTimeout(function() {
        moduleEl.readyState = 1;
        moduleEl.dispatchEvent(new CustomEvent('loadstart'));
        moduleEl.readyState = 4;
        moduleEl.dispatchEvent(new CustomEvent('load'));
        moduleEl.dispatchEvent(new CustomEvent('loadend'));
      }, 100);  // 100 ms
    }
  }
  
  /**
   * Create the Native Client <embed> element as a child of the DOM element
   * named "listener".
   *
   * @param {string} name The name of the example.
   * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
   * @param {string} path Directory name where .nmf file can be found.
   * @param {number} width The width to create the plugin.
   * @param {number} height The height to create the plugin.
   * @param {Object} attrs Dictionary of attributes to set on the module.
   */
  function createPage(pageNum, pageId, width, height) {
    var moduleEl = document.createElement('embed');
    moduleEl.setAttribute('id', pageId + 'nacl');
    moduleEl.setAttribute('width', width);
    moduleEl.setAttribute('height', height);
    moduleEl.setAttribute('plugin_type', 'page');
    moduleEl.setAttribute('page_num', pageNum);
    moduleEl.setAttribute('onload', 'graphics.view.pageDidLoad("' + pageId + '");');
    if (settings.type == 'pnacl') {
      moduleEl.setAttribute('src', settings.renderer.nmf);
    }

    var mimetype = settings.renderer.mimetype;
    moduleEl.setAttribute('type', mimetype);

    // The <EMBED> element is wrapped inside a <DIV>, which has both a 'load'
    // and a 'message' event listener attached.  This wrapping method is used
    // instead of attaching the event listeners directly to the <EMBED> element
    // to ensure that the listeners are active before the NaCl module 'load'
    // event fires.
    var pageDiv = document.getElementById(pageId);
    pageDiv.appendChild(moduleEl);

    // Request the offsetTop property to force a relayout. As of Apr 10, 2014
    // this is needed if the module is being loaded on a Chrome App's
    // background page (see crbug.com/350445).
    moduleEl.offsetTop;

    // Host plugins don't send a moduleDidLoad message. We'll fake it here.
    var isHost = isHostToolchain('win');
    if (isHost) {
      window.setTimeout(function() {
        moduleEl.readyState = 1;
        moduleEl.dispatchEvent(new CustomEvent('loadstart'));
        moduleEl.readyState = 4;
        moduleEl.dispatchEvent(new CustomEvent('load'));
        moduleEl.dispatchEvent(new CustomEvent('loadend'));
      }, 100);  // 100 ms
    }
  }

  /**
   * Add the default "load" and "message" event listeners to the element with
   * id "listener".
   *
   * The "load" event is sent when the module is successfully loaded. The
   * "message" event is sent when the naclModule posts a message using
   * PPB_Messaging.PostMessage() (in C) or pp::Instance().PostMessage() (in
   * C++).
   */
  function attachDefaultListeners() {
    var listenerDiv = document.getElementById('listener');
    listenerDiv.addEventListener('load', moduleDidLoad, true);
    listenerDiv.addEventListener('message', handleMessage, true);
    listenerDiv.addEventListener('error', handleError, true);
    listenerDiv.addEventListener('crash', handleCrash, true);
    if (typeof window.attachListeners !== 'undefined') {
      window.attachListeners();
    }
  }

  /**
   * Called when the NaCl module fails to load.
   *
   * This event listener is registered in createNaClModule above.
   */
  function handleError(event) {
    // We can't use common.naclModule yet because the module has not been
    // loaded.
    var moduleEl = document.getElementById('nacl_module');
    updateStatus('ERROR [' + moduleEl.lastError + ']');
  }

  /**
   * Called when the Browser can not communicate with the Module
   *
   * This event listener is registered in attachDefaultListeners above.
   */
  function handleCrash(event) {
    if (common.naclModule.exitStatus == -1) {
      updateStatus('CRASHED');
    } else {
      updateStatus('EXITED [' + common.naclModule.exitStatus + ']');
    }
    if (typeof window.handleCrash !== 'undefined') {
      window.handleCrash(common.naclModule.lastError);
    }
  }

  /**
   * Called when the NaCl module is loaded.
   *
   * This event listener is registered in attachDefaultListeners above.
   */
  function moduleDidLoad() {
    //common.naclModule = document.getElementById('nacl_module');
    //console.log('load event');
    if (common.decoder === null) {
      common.decoder = document.getElementById('decoder');
      var message = { message: messages.PPD_DOWNLOAD_START, args: 0 };
      common.decoder.postMessage(message);
      updateStatus('DOWNLOADING:0%');
    } else {
      //console.log('decoder === null');
    }
    //updateStatus('RUNNING');
    

    if (typeof window.moduleDidLoad !== 'undefined') {
      window.moduleDidLoad();
    }
  }

  /**
   * Hide the NaCl module's embed element.
   *
   * We don't want to hide by default; if we do, it is harder to determine that
   * a plugin failed to load. Instead, call this function inside the example's
   * "moduleDidLoad" function.
   *
   */
  function hideModule() {
    // Setting common.naclModule.style.display = "None" doesn't work; the
    // module will no longer be able to receive postMessages.
    common.naclModule.style.height = '0';
  }

  /**
   * Remove the NaCl module from the page.
   */
  function removeModule() {
    common.naclModule.parentNode.removeChild(common.naclModule);
    common.naclModule = null;
  }

  /**
   * Return true when |s| starts with the string |prefix|.
   *
   * @param {string} s The string to search.
   * @param {string} prefix The prefix to search for in |s|.
   */
  function startsWith(s, prefix) {
    // indexOf would search the entire string, lastIndexOf(p, 0) only checks at
    // the first index. See: http://stackoverflow.com/a/4579228
    return s.lastIndexOf(prefix, 0) === 0;
  }

  /** Maximum length of logMessageArray. */
  var kMaxLogMessageLength = 20;

  /** An array of messages to display in the element with id "log". */
  var logMessageArray = [];

  /**
   * Add a message to an element with id "log".
   *
   * This function is used by the default "log:" message handler.
   *
   * @param {string} message The message to log.
   */
  function logMessage(message) {
    logMessageArray.push(message);
    if (logMessageArray.length > kMaxLogMessageLength)
      logMessageArray.shift();

    document.getElementById('log').textContent = logMessageArray.join('\n');
    console.log(message);
  }

  /**
   */
  var defaultMessageTypes = {
    'alert': alert,
    'log': logMessage
  };

  /**
   * Called when the NaCl module sends a message to JavaScript (via
   * PPB_Messaging.PostMessage())
   *
   * This event listener is registered in createNaClModule above.
   *
   * @param {Event} message_event A message event. message_event.data contains
   *     the data sent from the NaCl module.
   */
  function handleMessage(message_event) {
    if (typeof message_event.data === 'string') {
      console.log('String message ' + message_event.data);
      // @TODO (ilia) fix it later
      /*
      for (var type in defaultMessageTypes) {
        if (defaultMessageTypes.hasOwnProperty(type)) {
          if (startsWith(message_event.data, type + ':')) {
            func = defaultMessageTypes[type];
            func(message_event.data.slice(type.length + 1));
            return;
          }
        }
      }
      */
    } else if (typeof message_event.data === 'object') {
      var message_data = message_event.data;
      
      switch (message_data.message) {
        // Document downloading
        case (messages.PPB_DOWNLOAD_PROGRESS):
          updateStatus('DOWNLOADING:' + message_data.args + '%');
          break;
        case (messages.PPB_DOWNLOAD_FINISHED):
          updateStatus('DOWNLOADING:finished');
          console.log('start decode');
          var message = { message: messages.PPD_DECODE_START, args: 0 };
          common.decoder.postMessage(message);
          break;
        // Document decoding
        case (messages.PPB_DECODE_FINISHED):
          updateStatus('DOWNLOADING:decoded');
          common.pages = message_data.args;
          onDocumentDecodeed(common.pages);
          break;
        // Page rendering
        case (messages.PPR_PAGE_READY):
          var renderer = document.getElementById(message_data.args);
          if (renderer == null) {
            var message = { message: messages.PPD_RELEASE_PAGE, args: message_data.args };
            common.decoder.postMessage(message);
          } else {
            var message = { message: messages.PPR_PAGE_READY, args: message_data.args };
            renderer.postMessage(message);
          }
          break;
        case (messages.PPD_GET_PAGE):
          var decoder = common.decoder;
          if (decoder != null) {
            var message = { message: messages.PPD_GET_PAGE, args: message_data.args };
            decoder.postMessage(message);
          }
          break;
        case (messages.PPR_SEND_PAGE):
          console.log(message_data.args.pageId + ' dataLength: ' + message_data.args.page.imageData.byteLength);
          //var arr = new Int8Array(message_data.args.page.imageData);
          //console.log(arr);
          var decoder = common.decoder;
          if (decoder != null) {
            var message = { message: messages.PPD_RELEASE_PAGE, args: message_data.args.pageId };
            decoder.postMessage(message);
          }
          var renderer = document.getElementById(message_data.args.pageId);
          if (renderer != null) {
            console.log('sending page');
            //console.log(message_data.args.page.imageData[0]);
            var message = { message: messages.PPR_SEND_PAGE, args: message_data.args.page };
            renderer.postMessage(message);
          }
          break;
          
          
          
        // Error handling
        case (messages.PPB_PLUGIN_ERROR):
          console.log(message_data);
          break;
        default:
          console.log('Unhandled message');
          console.log(message_data);
      }
      
      
      
      
      return;

      if (message.name === 'error') {
        console.log('ERROR');
      }

      if (message.target === 'browser') {
        // Handle notifications
        if (message.type === 'notify') {
          // Errors
          if (message.name === 'error') { 
            console.log('Error');
            console.log(message.args);
          // Document download
          } else if (message.name === 'download') {
            if (message.args === 'finished') {
              updateStatus('DOWNLOADING:finished');
              var message = { target: 'decoder', type: 'command', name: 'decode', args: 'start' };
              common.decoder.postMessage(message);
            } else {
              if (typeof(message.args) === 'number') {
                updateStatus('DOWNLOADING:' + message.args + '%');
              }
            }
          }
        }
        
        // Handle data passing
        if (message.type === 'object') {
          if (message.name === 'pages') {
            common.pages = message.args;
            onDocumentDecodeed(common.pages);
          }
        }
      } else {
        if (message.type === 'notify') {
          if (message.name === 'decoded') {
            console.log('Page ' + message.target + ' decoded');
            var pageId = message.target;
            var pageEl = document.getElementById(pageId);
            var message = { target: pageId, type: 'notify', name: 'decoded', args: message.args };
            console.log(message);
            pageEl.postMessage(message);
          }
        }
      }
      //console.log(message_event.data);
    }

    if (typeof window.handleMessage !== 'undefined') {
      window.handleMessage(message_event);
      return;
    }

    //logMessage('Unhandled message: ' + message_event.data);
  }

  /**
   * Called when the DOM content has loaded; i.e. the page's document is fully
   * parsed. At this point, we can safely query any elements in the document via
   * document.querySelector, document.getElementById, etc.
   *
   * @param {string} name The name of the example.
   * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
   * @param {string} path Directory name where .nmf file can be found.
   * @param {number} width The width to create the plugin.
   * @param {number} height The height to create the plugin.
   * @param {Object} attrs Optional dictionary of additional attributes.
   */
  function domContentLoaded(name, tool, path, width, height, attrs) {
    // If the page loads before the Native Client module loads, then set the
    // status message indicating that the module is still loading.  Otherwise,
    // do not change the status message.
    updateStatus('Page loaded.');
    var tool = 'win';
    if (!browserSupportsNaCl(tool)) {
      updateStatus(
          'Browser does not support NaCl (' + tool + '), or NaCl is disabled');
    } else if (common.naclModule == null) {
      updateStatus('Creating embed: ' + tool);

      // We use a non-zero sized embed to give Chrome space to place the bad
      // plug-in graphic, if there is a problem.
      /*
      width = typeof width !== 'undefined' ? width : 200;
      height = typeof height !== 'undefined' ? height : 200;
      */
      attachDefaultListeners();
      createDecoder();
      //createNaClModule(name, tool, path, width, height, attrs);
    } else {
      // It's possible that the Native Client module onload event fired
      // before the page's onload event.  In this case, the status message
      // will reflect 'SUCCESS', but won't be displayed.  This call will
      // display the current message.
      updateStatus('Waiting.');
    }
  }

  /** Saved text to display in the element with id 'statusField'. */
  var statusText = 'NO-STATUSES';

  /**
   * Set the global status message. If the element with id 'statusField'
   * exists, then set its HTML to the status message as well.
   *
   * @param {string} opt_message The message to set. If null or undefined, then
   *     set element 'statusField' to the message from the last call to
   *     updateStatus.
   */
  function updateStatus(opt_message) {
    if (opt_message) {
      statusText = opt_message;
    }
    var statusField = document.getElementById('statusField');
    if (statusField) {
      statusField.innerHTML = statusText;
    }
  }

  // The symbols to export.
  return {
    /** A reference to the NaCl module, once it is loaded. */
    naclModule: null,
    decoder: null,
    pages: [],

    attachDefaultListeners: attachDefaultListeners,
    domContentLoaded: domContentLoaded,
    //createNaClModule: createNaClModule,
    createDecoder: createDecoder,
    createPage: createPage,
    hideModule: hideModule,
    removeModule: removeModule,
    logMessage: logMessage,
    updateStatus: updateStatus
  };

}());

// Listen for the DOM content to be loaded. This event is fired when parsing of
// the page's document has finished.
document.addEventListener('DOMContentLoaded', function() {
  //loadSettings('ppapi', 'http://localhost/docs/1.djvu');
  loadSettings();

  var body = document.body;

  // The data-* attributes on the body can be referenced via body.dataset.
  if (body.dataset) {
    var loadFunction;
    if (!body.dataset.customLoad) {
      loadFunction = common.domContentLoaded;
    } else if (typeof window.domContentLoaded !== 'undefined') {
      loadFunction = window.domContentLoaded;
    }

    // From https://developer.mozilla.org/en-US/docs/DOM/window.location
    var searchVars = {};
    if (window.location.search.length > 1) {
      var pairs = window.location.search.substr(1).split('&');
      for (var key_ix = 0; key_ix < pairs.length; key_ix++) {
        var keyValue = pairs[key_ix].split('=');
        searchVars[unescape(keyValue[0])] =
            keyValue.length > 1 ? unescape(keyValue[1]) : '';
      }
    }

    if (loadFunction) { 
      loadFunction();
    }
    // Don't need to parse arguments @TODO(ilia) remove this later
    /*
    if (loadFunction) {
      var toolchains = body.dataset.tools.split(' ');
      var configs = body.dataset.configs.split(' ');

      var attrs = {};
      if (body.dataset.attrs) {
        var attr_list = body.dataset.attrs.split(' ');
        for (var key in attr_list) {
          var attr = attr_list[key].split('=');
          var key = attr[0];
          var value = attr[1];
          attrs[key] = value;
        }
      }

      var tc = toolchains.indexOf(searchVars.tc) !== -1 ?
          searchVars.tc : toolchains[0];

      // If the config value is included in the search vars, use that.
      // Otherwise default to Release if it is valid, or the first value if
      // Release is not valid.
      if (configs.indexOf(searchVars.config) !== -1)
        var config = searchVars.config;
      else if (configs.indexOf('Release') !== -1)
        var config = 'Release';
      else
        var config = configs[0];

      var pathFormat = body.dataset.path;
      var path = pathFormat.replace('{tc}', tc).replace('{config}', config);

      isTest = searchVars.test === 'true';
      isRelease = path.toLowerCase().indexOf('release') != -1;

      loadFunction();
      //loadFunction(body.dataset.name, tc, path, body.dataset.width,
      //             body.dataset.height, attrs);
    }
    */
  }
});
