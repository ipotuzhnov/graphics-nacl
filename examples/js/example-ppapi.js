// Listen for the DOM content to be loaded. This event is fired when parsing of
// the page's document has finished.
var decoder;

function updateStatus(opt_message) {
  if (opt_message) {
    statusText = opt_message;
  }
  var statusField = document.getElementById('statusField');
  if (statusField) {
    statusField.innerHTML = statusText;
  }
}

document.addEventListener('DOMContentLoaded', function() {
  // example
  var type = 'ppapi';
  var url = 'http://localhost/docs/1.djvu';
  //var url = 'http://127.0.0.1/docs/djvu?nd=123124124';
  var settings = {type: type, url: url};
  decoder = new DJVUDecoder(
    settings, 
    function(progress) {
      // Update progress
      updateStatus('DOWNLOADING:' + progress + '%');
      //console.log(progress);
    },
    function(err, pages) {
      if (err) throw err;
      if (pages === undefined) throw 'Pages array in undefiend';
      console.log('Document is decoded');
      console.log('and has ' + pages.length + ' pages');
      
      if (pages.length > 0) {
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
    }
  );
});