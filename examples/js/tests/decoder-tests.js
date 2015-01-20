var tests = function(decoder) {
  var asyncDecoders = {};
  var module = {
    decoded: 0,
    length: 0
  };

  function testGetPageText(decoder) {
    if (decoder === undefined) throw 'decoder is not defined';
    var page_text_settings = { pageNumber: 0 };
    decoder.getPageText(page_text_settings, function(err, pageText) {
      if (err) throw err;
      // Go through page's text array
      for (var i = 0; i < pageText.length; i++) {
        var word = pageText[i].word;
        var rect = pageText[i].rect;
        var word_rect = 'PageText{ word: ' + word + ' rect: { left: ' + rect.left + ' top: ' + rect.top + ' right: ' + rect.right + ' bottom: ' + rect.bottom + ' } }';
        console.log(word_rect);
      }
    });
  }

  function testGetPage(decoder, pages) {
    if (decoder === undefined) throw 'decoder is not defined';
    if (pages.length <= 0) throw 'Pages array is empty';
    // Create settings for the page
    var width = Math.round(pages[0].width / 1);
    var height = Math.round(pages[0].height / 1);
    var size = { width: width, height: height };
    var frame = { left: 100, top: 100, right: 200, bottom: 200 };
    var page_settings = { pageNumber: 0, size: size, frame: frame };
    decoder.getPage(page_settings, function(err, page) {
      if (err) throw err;
      // Create image element
      var imageEl = document.createElement('img');
      imageEl.setAttribute('id', 'examplePageImg0');
      imageEl.setAttribute('src', page.imageData);
      imageEl.setAttribute('width', page.width);
      imageEl.setAttribute('height', page.height);
      // Add image to the body
      document.body.appendChild(imageEl);
      
    });
  }

  function AsyncDecoder(id, decoder, pages, numberOfThreads, numberOfPagesToDecode) {
    this.id = id;
    this.decoder = decoder;
    this.pages = pages;
    this.numberOfThreads = numberOfThreads;
    this.numberOfPagesToDecode = numberOfPagesToDecode;
    this.decoded = 0;
  }
  
   AsyncDecoder.prototype.getPage = function(pageNumber, callback) {
    // Create settings for the page
    var size = { width: 100, height: 100 };
    var page_settings = { pageNumber: pageNumber, size: size };
    this.decoder.getPage(page_settings, function(err, page) {
      if (err) throw err;
      // Create image element
      var id = 'asyncGetPage' + pageNumber + 'decodedBy:' + this.id;
      var imageEl = document.createElement('img');
      imageEl.setAttribute('id', id);
      imageEl.setAttribute('src', page.imageData);
      imageEl.setAttribute('width', page.width);
      imageEl.setAttribute('height', page.height);
      // Add image to the body
      document.body.appendChild(imageEl);
      if (pageNumber == (this.numberOfPagesToDecode - 1)) console.log('decoded the last page');
      callback();
    }.bind(this));
  }

  AsyncDecoder.prototype.getNextPages = function() {
    // Decode
    if (this.decoded % this.numberOfThreads === 0) {
      var offset = this.decoded;
      for (var i = 0; i < this.numberOfThreads; i++) {
        var pageNumber = offset + i;
        if (pageNumber > this.numberOfPagesToDecode - 1) return;
        this.getPage(pageNumber, function() {
          this.decoded++;
          this.getNextPages();
        }.bind(this));
      }
    }
  }

  AsyncDecoder.prototype.getPagesAsync = function() {
    this.getNextPages();
  }

  function testAsyncGetPage(decoder, pages, numberOfThreads, numberOfPagesToDecode) {
    if (decoder === undefined) throw 'decoder is not defined';
    if (pages.length <= 0) throw 'Pages array is empty';
    if (numberOfPagesToDecode === undefined) numberOfPagesToDecode = pages.length;
    if (numberOfPagesToDecode > pages.length) numberOfPagesToDecode = pages.length;
    if (numberOfThreads === undefined) numberOfThreads = 5;
    // Create new asynchronous decoder
    var id = makeUniqueId();
    var asyncDecoder = new AsyncDecoder(id, decoder, pages, numberOfThreads, numberOfPagesToDecode);
    asyncDecoders[id] = { asyncDecoder: asyncDecoder };
    asyncDecoder.getPagesAsync();
  }
  
  /**
   * Generates unique id.
   */
  function makeUniqueId() {
    var res = '';
    var possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';

    for (var i = 0; i < 16; i++)
      res += possible.charAt(Math.floor(Math.random() * possible.length));
      
    // Check if id is unique
    //if (this.tmp[res] !== undefined)
    //  this.makeUniqueId();
    return res;
  }
  
  
  return {
    testGetPageText: testGetPageText,
    testGetPage: testGetPage,
    testAsyncGetPage: testAsyncGetPage,
    AsyncDecoder: AsyncDecoder
  }
}();