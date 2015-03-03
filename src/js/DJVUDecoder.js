/**
 * Wrapper class for decoding DjVu documents.
 * 
 * @class {DJVUDecoder} 
 * @constructor {DJVUDecoder}
 * @param {Object} settings Dictionary of settings to set on the decoder.
 *    Settings attributes:
 *    @property {string} type The type of plugin "pnacl" or "ppapi".
 *    @property {string} url The url where .djvu document can be found.
 *    Optional attributes:
 *    @property {string} nmf The url where .nmf file can be found. PNaCl only.
 * @param {function} progress Downloading progress notification callback.
 * @param {function} callback Error callback.
 */
function DJVUDecoder(settings, progress, callback) {
  this.progress = progress;
  this.callback = callback;
  this.reloading = false;
  if (!settings) return callback('Settings object is not defined');
  // Set plugin mimetype
  if (!settings.type) return callback('Type is not defined in settings object');
  this.type = settings.type;
  switch (this.type) {
    case ('pnacl'):
      this.mimetype = 'application/x-pnacl';
      if (settings.debug !== undefined) {
        if (settings.debug === true)
          this.mimetype = 'application/x-nacl';
      }
      // Set .nmf location for PNaCl plugin
      if (!settings.nmf) return callback('The location of .nmf file is not defined in settings object');
      this.nmf = settings.nmf;
      break;
    case ('ppapi'):
      this.mimetype = 'application/x-ppapi-decoder';
      break;
    default:
      return callback(this.type + ' is not a valid type for plugin');
  }
  // Check if the browser supports mimetype.
  if (navigator.mimeTypes[this.mimetype] === undefined) return callback(this.mimetype + ' is not supported by the browser');
  // Set document url
  if (!settings.url) return callback('Url is not defined in settings object');
  this.url = settings.url;
  // Create log element
  var logEl = document.createElement('div');
  document.body.appendChild(logEl);
  this.log = logEl;
  // Create exception element
  var exceptionEl = document.createElement('div');
  document.body.appendChild(exceptionEl);
  this.exception = exceptionEl;
  // Create event listener
  var listenerEl = document.createElement('div');
  document.body.appendChild(listenerEl);
  this.listener = listenerEl;
  this.attachDefaultListeners();
  // Create NaCl module
  this.createNaClModule();
  console.log('module constructor finished');
}

/**
 * Dictionary of decoded pages
 * @property {Object} pages.
 *    Page object structure:
 *    @property {string} width The width of the page.
 *    @property {string} height The height of the page.
 *    @property {string} dpi The dpi of the page.
 * TODO (ilia) change visibility to private.
 */
DJVUDecoder.prototype.pages = {};

/**
 * Dictionary of temporary pages.
 * @property {Object} jobs The dictionary of decoding page.
 *    @property {function} callback The callback function.
 */
DJVUDecoder.prototype.jobs = {};

/**
 * Add the default "load", "message", "error" and "crash" event listeners to the element with
 * id "listener".
 *
 * The "load" event is sent when the module is successfully loaded. The
 * "message" event is sent when the naclModule posts a message using
 * PPB_Messaging.PostMessage() (in C) or pp::Instance().PostMessage() (in
 * C++).
 */
DJVUDecoder.prototype.attachDefaultListeners = function() {
  // Bind all callbacks with this 
  this.listener.addEventListener('load', this.moduleDidLoad.bind(this), true);
  this.listener.addEventListener('message', this.handleMessage.bind(this), true);
  this.listener.addEventListener('error', this.handleError.bind(this), true);
  this.listener.addEventListener('crash', this.handleCrash.bind(this), true);
  if (typeof window.attachListeners !== 'undefined') {
    window.attachListeners();
  }
}

/**
 * Called when the NaCl module is loaded.
 *
 * This event listener is registered in attachDefaultListeners above.
 */
DJVUDecoder.prototype.moduleDidLoad = function() {
  console.log('load');
  if (this.module === undefined) {
    var decoderEl = document.getElementById('decoder');
    if (decoderEl === undefined) { 
      console.log('decoder el is not defined');
      return;
    }
    this.module = decoderEl
    var message = { message: messages.PPD_DOWNLOAD_START, args: 0 };
    this.module.postMessage(message);
    console.log('NaCl module was successfully loaded');
  } else {
    console.log('this.module is already set');
  }
  
  if (typeof window.moduleDidLoad !== 'undefined') {
    window.moduleDidLoad();
  }
}

/**
 * Called when the NaCl module fails to load.
 *
 * This event listener is registered in createNaClModule above.
 */
DJVUDecoder.prototype.handleError = function(event) {
  // We can't use common.naclModule yet because the module has not been
  // loaded.
  var moduleEl = document.getElementById('decoder');
  
  console.log('ERROR [' + moduleEl.lastError + ']');
  console.log(event);
}

/**
 * Called when the NaCl module sends a message to JavaScript (via
 * PPB_Messaging.PostMessage())
 *
 * This event listener is registered in createNaClModule above.
 *
 * @param {Event} message_event A message event. message_event.data contains
 *     the data sent from the NaCl module.
 */
DJVUDecoder.prototype.handleMessage = function(message_event) {
  if (typeof message_event.data === 'string') {
    console.log('String message ' + message_event.data);
  } else if (typeof message_event.data === 'object') {
    var message_data = message_event.data;
    
    switch (message_data.message) {
      // Document downloading
      case (messages.PPB_DOWNLOAD_PROGRESS):
        this.progress(message_data.args);
        break;
      case (messages.PPB_DOWNLOAD_FINISHED):
        var message = { message: messages.PPD_DECODE_START, args: 0 };
        this.module.postMessage(message);
        break;
      // Document decoding
      case (messages.PPB_DECODE_FINISHED):
        this.pages = message_data.args;
        if (!this.reloading) this.callback(null, this.pages);
        break;
      // Page rendering
      case (messages.PPB_PAGE_READY):
        var id = message_data.args;
        if (this.jobs[id] === undefined) {
          var message = { message: messages.PPD_RELEASE_PAGE, args: message_data.args };
          this.module.postMessage(message);
        } else {
          var message = { message: messages.PPD_GET_PAGE_AS_BASE64, args: message_data.args };
          this.module.postMessage(message);
        }
        break;
      case (messages.PPB_SEND_PAGE_AS_BASE64):
        var id = message_data.args.pageId;
        if (this.jobs[id] !== undefined) {
          var page = message_data.args.page;
          page.imageData = 'data:image/png;base64,' + page.imageData;
          this.jobs[id].callback(null, page);
          // Clean up
          delete this.jobs[id];
        }
        if (this.module != null) {
          var message = { message: messages.PPD_RELEASE_PAGE, args: message_data.args.pageId };
          this.module.postMessage(message);
        }
        break;
      case (messages.PPB_SEND_PAGE_TEXT):
        var id = message_data.args.pageId;
        if (this.jobs[id] !== undefined) {
          var pageText = message_data.args.pageText;
          this.jobs[id].callback(null, pageText);
          // Clean up
          delete this.jobs[id];
        }
        break;
      // Error handling
      case (messages.PPB_PLUGIN_ERROR):
        console.log(message_data);
        break;
      // Log handling
      case (messages.PPB_PLUGIN_LOG):
        //console.log(message_data.args);
        //this.log.innerHTML = this.log.innerHTML + message_data.args;
        break;
      // Exception handling
        this.crashed = true;
        document.getElementById('json').value = message_data.args;
        crash_info = JSON.parse(message_data.args);
        //updateStatus('Crash Reported');
        //src = common.naclModule.getAttribute('path');
        //fetchMap(src + '/debugging_' + crash_info['arch'] + '.map', crash_info);
        this.exception.innerHTML = crash_info;
      default:
        console.log('Unhandled message');
        console.log(message_data);
    }
  }
  if (typeof window.handleMessage !== 'undefined') {
    window.handleMessage(message_event);
  }
}

/**
 * Called when the Browser can not communicate with the Module
 *
 * This event listener is registered in attachDefaultListeners above.
 */
DJVUDecoder.prototype.handleCrash = function(event) {
  if (this.module.exitStatus == -1) {
    console.log('CRASHED');
  } else {
    // Reload module and restart all jobs.
    console.log('EXITED [' + this.module.exitStatus + ']');
    /*
    this.reloading = true;
    if (this.module !== undefined) {
      var l = this.listener;
      this.listener.removeChild(this.module);
      this.module = undefined;
    }
    this.createNaClModule();
    for (var job in this.jobs) {
      this.module.postMessage(job.message);
    }
    */
  }
  if (typeof window.handleCrash !== 'undefined') {
    console.log(event);
    window.handleCrash(this.module.lastError);
  }
}

/**
 * Create the Native Client <embed> element as a child of the DOM element
 * named "listener".
 */
DJVUDecoder.prototype.createNaClModule = function() {
  console.log('create nacl module');
  var moduleEl = document.createElement('embed');
  moduleEl.setAttribute('id', 'decoder');
  moduleEl.setAttribute('width', 0);
  moduleEl.setAttribute('height', 0);
  moduleEl.setAttribute('plugin_type', 'decoder');
  moduleEl.setAttribute('docsrc', this.url);
  if (this.type == 'pnacl') {
    moduleEl.setAttribute('src', this.nmf);
  }

  moduleEl.setAttribute('type', this.mimetype);

  // The <EMBED> element is wrapped inside a <DIV>, which has both a 'load'
  // and a 'message' event listener attached.  This wrapping method is used
  // instead of attaching the event listeners directly to the <EMBED> element
  // to ensure that the listeners are active before the NaCl module 'load'
  // event fires.
  this.listener.appendChild(moduleEl);

  // Request the offsetTop property to force a relayout. As of Apr 10, 2014
  // this is needed if the module is being loaded on a Chrome App's
  // background page (see crbug.com/350445).
  moduleEl.offsetTop;

  // Host plugins don't send a moduleDidLoad message. We'll fake it here.
  if (this.type == 'ppapi') {
    window.setTimeout(function() {
      moduleEl.readyState = 1;
      moduleEl.dispatchEvent(new CustomEvent('loadstart'));
      moduleEl.readyState = 4;
      moduleEl.dispatchEvent(new CustomEvent('load'));
      moduleEl.dispatchEvent(new CustomEvent('loadend'));
      console.log('module created');
    }, 100);  // 100 ms
  }
}

/**
 * Request page as base64 string. Callback reterns decoded page.
 * If frame is not set then page is decoded in full size and sent back through 
 * callback function. In other case method returns part of the page 
 * that specified by frame.
 * @param {Object} settings Dictionary of page's settings
 *    Settings attributes:
 *    @property {numeber} pageNumber The number of the page.
 *    @property {Object} size The size of the page.
 *        @property {number} width The width of the page.
 *        @property {number} height The height of the page.
 *    Optional attributes:
 *    @property {Object} frame The frame on the page to be decoded.
 *        @property {number} left The x coordinate of the top left corner.
 *        @property {number} top The y coordinate of the top left corner.
 *        @property {number} right The x coordinate of the bottom right corner.
 *        @property {number} bottom The y coordinate of the bottom right corner.
 * @param {function} callback The callback function.
 *    @param {string} err The error string.
 *    @param {Object} page The page object.
 *        @property {number} width The width of the page.
 *        @property {number} height The height of the page.
 *        @property {string} data The base64 encoded .png image of the page.
 *
 * TODO (ilia) add {bool} getWithText flag to the settings object.
 */
DJVUDecoder.prototype.getPage = function(settings, callback) {
  if (!settings) return callback('Page settings object is not defined');
  if (settings.pageNumber === undefined) return callback('Page settings.pageNumber is not defined');
  var pageNumber = settings.pageNumber;
  if (settings.size === undefined) return callback('Page settings.size is not defined');
  var size = settings.size;
  var frame = settings.frame ? frame = settings.frame : null;
  // Start decoding
  // Generate an id for page
  var id = this.makeUniqueId();
  var args = { pageId: id, pageNum: pageNumber, size: size, frame: frame };
  var message = { message: messages.PPD_DECODE_PAGE, args: args };
  this.jobs[id] = { message: message, callback: callback };
  this.module.postMessage(message);
}

/**
 * Request text layer of the page.
 * @param {Object} settings Dictionary of page's settings
 *    Settings attributes:
 *    @property {numeber} pageNumber The number of the page.
 * @param {function} callback The callback function.
 *    @param {string} err The error string.
 *    @param {Object} pageText The array of words in rectangles.
 *        @property {string} word The word in the rectangle.
 *        @property {Object} rect The rectangle.
 *            Rectangle attributes:
 *            @property {number} left The x coordinate of the top left corner.
 *            @property {number} top The y coordinate of the top left corner.
 *            @property {number} right The x coordinate of the bottom right corner.
 *            @property {number} bottom The y coordinate of the bottom right corner.
 */
DJVUDecoder.prototype.getPageText = function(settings, callback) {
  if (!settings) return callback('Page settings object is not defined');
  if (settings.pageNumber === undefined) return callback('Page settings.pageNumber is not defined');
  var pageNumber = settings.pageNumber;
  // Request page's text
  var id = this.makeUniqueId();
  var args = { pageId: id, pageNum: pageNumber };
  var message = { message: messages.PPD_GET_PAGE_TEXT, args: args };
  this.jobs[id] = { message: message, callback: callback };
  this.module.postMessage(message);
}

/**
 * Generates unique id.
 */
DJVUDecoder.prototype.makeUniqueId = function() {
  var res = '';
  var possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';

  for (var i = 0; i < 16; i++)
    res += possible.charAt(Math.floor(Math.random() * possible.length));
    
  // Check if id is unique
  if (this.jobs[res] !== undefined)
    this.makeUniqueId();
  return res;
}
