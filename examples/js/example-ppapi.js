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
      
      // Test get page text
      //testGetPageText();
      
      // Test page decoding
      //testGetPage(pages);
      
      // Anync page decoding. 
      // 5 pages by itteration.
      tests.testAsyncGetPage(decoder, pages, 5);
      // Anync page decoding. 
      // tests.testAsyncGetPage(decoder, pages, 7, 15);
      //tests.testAsyncGetPage(decoder, pages, 5, 15);
    }
  );
});
































