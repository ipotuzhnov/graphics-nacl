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
  var type = 'pnacl';
  var url = 'http://172.16.2.114:88/graphics_nacl/1.djvu';
  var nmf = '/graphics-nacl-git/graphics-nacl-decoder-debug.nmf';
  var settings = { type: type, url: url, nmf: nmf, debug: true };
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
      tests.testAsyncGetPage(decoder, pages, 1);
      //tests.testAsyncGetPage(decoder, pages, 3);
      //tests.testAsyncGetPage(decoder, pages, 4);
      //tests.testAsyncGetPage(decoder, pages, 5);
      // Anync page decoding. 
      // tests.testAsyncGetPage(decoder, pages, 7, 15);
      //tests.testAsyncGetPage(decoder, pages, 5, 15);
    }
  );
});

