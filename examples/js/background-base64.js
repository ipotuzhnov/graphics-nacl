// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Globals =(
var maxWidth = 0;
var maxHeight = 0;

function viewDidScroll() {
  //console.log('view did scroll');
  //var view = document.getElementById('scrollView');
  if (graphics.scrollView !== null) {
    console.log(graphics.scrollView.scrollTop);
  } else {
    console.log('error: scrollView == null');
  }
}

function onDocumentDecodeed(pages) {
  var scale = 0.2;
  var scrollBarSize = 20;
  
  var listenerDiv = document.getElementById('listener');

  var margin = 10;
  var view = { width: 0, height: 0 };
  for (var pageNum = 0; pageNum < pages.length; pageNum++) {
    view.height += Math.floor(pages[pageNum].height / 5) + 2 * margin;
    if (pages[pageNum].width > maxWidth) {
      maxWidth = pages[pageNum].width;
    }
    if (pages[pageNum].height > maxHeight) {
      maxHeight = pages[pageNum].height;
    }
  }
  view.width += Math.floor(maxWidth * scale) + 2 * margin;
  
  var mainViewEl = document.createElement('div');
  mainViewEl.setAttribute('name', 'mainView');
  mainViewEl.setAttribute('id', 'mainView');  
  //mainViewEl.setAttribute('style', 'float: left; width: ' + (Math.floor(maxWidth / 3) + 2 * margin) + 'px; height: ' + (Math.floor(maxHeight / 3) + 100) + 'px; border-width: 1px; border-style: solid; border-color: rgba(0,0,0,.2); background-color:#13b4ff; ');
  var mainViewStyle = 'float: left;';
  mainViewStyle += 'width: ' + (Math.floor(maxWidth * scale) + 2 * margin + scrollBarSize) + 'px;';
  mainViewStyle += 'height: ' + (Math.floor(maxHeight * scale) + 100) + 'px;';
  mainViewStyle += 'background-color:#13b4ff;';
  mainViewEl.setAttribute('style', mainViewStyle);
  //mainViewEl.setAttribute('width', (Math.floor(maxWidth * scale) + 2 * margin) + 'px');
  //mainViewEl.setAttribute('height', (Math.floor(maxHeight * scale) + 100) + 'px');
    //margin: 5px 5px 5px 5px;
  //mainViewEl.innerHTML += 'Hello';
  listenerDiv.appendChild(mainViewEl);
  
  var scrollViewEl = document.createElement('div');
  scrollViewEl.setAttribute('name', 'scrollView');
  scrollViewEl.setAttribute('id', 'scrollView');
  //scrollViewEl.setAttribute('width', view.width + 'px');
  //scrollViewEl.setAttribute('height', view.height + 'px');
  var scrollViewStyle = 'float: left;';
  scrollViewStyle += 'width: ' + (Math.floor(maxWidth * scale) + 2 * margin + scrollBarSize) + 'px;';
  scrollViewStyle += 'height: ' + (Math.floor(maxHeight * scale) + 100) + 'px;';
  scrollViewStyle += 'background-color:#00FFFF;';
  scrollViewStyle += 'overflow-y: scroll;';
  scrollViewEl.setAttribute('style', scrollViewStyle);
  scrollViewEl.setAttribute('onscroll','graphics.view.viewDidScroll();');
  mainViewEl.appendChild(scrollViewEl);
  
  graphics.view.element = scrollViewEl;
  //graphics.scrollView = scrollViewEl;
  
  var pageOffsetY = 0;
  for (var pageNum = 0; pageNum < pages.length; pageNum++) {
    var pageId = 'pageView' + pageNum;
  
    var pageViewEl = document.createElement('div');
    pageViewEl.setAttribute('name', pageId);
    pageViewEl.setAttribute('id', pageId);
    var pageViewStyle = 'float: left;';
    //pageViewStyle += 'position: relative;';
    //pageViewStyle += 'top: ' + pageOffsetY + 'px;';
    //pageOffsetY += Math.floor(pages[pageNum].height / 3) + 2 * margin;
    pageViewStyle += 'width: ' + Math.floor(pages[pageNum].width * scale) + 'px;';
    pageViewStyle += 'height: ' + Math.floor(pages[pageNum].height * scale) + 'px;';
    var addMargin = Math.floor((maxWidth - pages[pageNum].width) * scale / 2);
    // top right bottom left
    // (top and bottom) and (left and right)
    pageViewStyle += 'margin: ' + margin + 'px ' + (margin + addMargin) + 'px;';
    pageViewStyle += 'background-color:#ff00ff;';
    pageViewEl.setAttribute('style', pageViewStyle);
    scrollViewEl.appendChild(pageViewEl);
    
    if (graphics.pages[pageId] == null) {
      graphics.pages[pageId] = { element: pageViewEl, module: null, img: null, page: null };
    }
  }
  
  /*
  var moduleEl = document.createElement('div');
    moduleEl.setAttribute('name', 'nacl_module');
    moduleEl.setAttribute('id', 'nacl_module');
    moduleEl.setAttribute('width', width);
    moduleEl.setAttribute('height', height);
    moduleEl.setAttribute('src', path + '/' + name + '.nmf');

    var mimetype = 'application/x-ppapi';
    moduleEl.setAttribute('type', mimetype);
*/

  console.log('Document is decoded and has ' + pages.length + ' pages');
  graphics.view.viewDidScroll();
  /*
  width: 50px;
    height: 200px;
    overflow-y: scroll;
    */
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
  function createPageAsBase64(pageNum, pageId, width, height) {
    var moduleEl = document.createElement('img');
    moduleEl.setAttribute('id', pageId + 'img');
    //moduleEl.setAttribute('src', "");
    moduleEl.setAttribute('width', width);
    moduleEl.setAttribute('height', height);
    moduleEl.setAttribute('page_num', pageNum);

    var pageDiv = document.getElementById(pageId);
    pageDiv.appendChild(moduleEl);

    // Request the offsetTop property to force a relayout. As of Apr 10, 2014
    // this is needed if the module is being loaded on a Chrome App's
    // background page (see crbug.com/350445).
    moduleEl.offsetTop;
  }

var graphics = ( function() {
  var view = ( function() {
    function viewDidScroll() {
      if (graphics.view.element !== null) {
        //console.log('view did scroll: ' + graphics.view.element.scrollTop);
        var offset = graphics.view.element.scrollTop;
        
        // Update page visibility
        var scale = 0.2;
        var scrollBarSize = 20;
        var margin = 10;
        //var pages = common.pages;
        var pageBounds = {top: 0, bottom: 0};
        var pageOffset = margin;
        var screen = {top: offset, bottom: offset + Math.floor(maxHeight * scale) + 100};
        //var screen = {top: offset, bottom: offset + Math.floor(maxHeight * scale) - 500};
        //console.log(screen);
        //for (var pageNum = 0; pageNum < common.pages.length; pageNum += 2000) {
        for (var pageNum = 0; pageNum < common.pages.length; pageNum++) {
          pageBounds.top = pageOffset;
          pageBounds.bottom = pageBounds.top + Math.floor(common.pages[pageNum].height * scale);
          // Change pageBounds offset
          pageOffset = pageBounds.bottom + 2 * margin;
          
          var pageId = 'pageView' + pageNum;
          // If page is on screen then create new img element
          if (pageBounds.top < screen.bottom && pageBounds.bottom > screen.top) {
            if (graphics.pages[pageId].img == null) {
              createPageAsBase64(pageNum, pageId, Math.floor(common.pages[pageNum].width * scale), Math.floor(common.pages[pageNum].height * scale));
              graphics.pages[pageId].img = document.getElementById(pageId + 'img');
              console.log('Create new img: ' + pageId + 'img');
              console.log(pageBounds);
              var pageIdImg = pageId + 'img';
              var pageWidth = Math.floor(common.pages[pageNum].width * scale);
              var pageHeight = Math.floor(common.pages[pageNum].height * scale);
              var args = { pageId: pageIdImg, pageNum: pageNum, width: pageWidth, height: pageHeight };
              var message = { message: messages.PPD_DECODE_PAGE, args: args };
              common.decoder.postMessage(message);
              //var message = { target: 'decoder', type: 'command', name: 'decode', args: args };
              //common.decoder.postMessage(message);
            } else { //if (graphics.pages[pageId].page != null) {
              var img = document.getElementById(pageId + 'img');
              if (img == null) {
                continue;
              }
              if (img.src != "") {
                continue;
              }
              var page = graphics.pages[pageId].page;
              if (page == null) {
                continue;
              }
              img.src = "data:image/png;base64," + page.imageData;
            }
          // If page is not on screen then remode img module element
          } else {
            var img = graphics.pages[pageId].img;
            if (img != null) {
              graphics.pages[pageId].element.removeChild(graphics.pages[pageId].img);
              graphics.pages[pageId].img = null;
              console.log('Remove out of screen img: ' + pageId + 'img');
              console.log(pageBounds);
            }
          }
        }
      }
    }
    
    // The symbols to export.
    return {
      element: null,
      viewDidScroll: viewDidScroll
    }
  } ());
  
  // The symbols to export.
  return {
    /** A reference to the View. */
    scrollView: null,
    view: view,
    decoder: null,
    pages: {}

    //attachDefaultListeners: attachDefaultListeners,
    //domContentLoaded: domContentLoaded,
    //createNaClModule: createNaClModule,
    //createDecoder: createDecoder,
    //createPage: createPage,
    //hideModule: hideModule,
    //removeModule: removeModule,
    //logMessage: logMessage,
    //updateStatus: updateStatus
  };
} ());

//chrome.app.runtime.onLaunched.addListener(onLaunched);
