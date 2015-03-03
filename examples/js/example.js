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
  var url = 'http://127.0.0.1:88/graphics_nacl/1.djvu';
  var nmf = '../graphics-nacl-decoder.nmf';
  var settings = { type: type, url: url, nmf: nmf};
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

      //tests.testAsyncGetPage(decoder, pages, 5, 12);

      /*
      tests.testAsyncGetPage(decoder, pages, 5);
      tests.testAsyncGetPage(decoder, pages, 6);
      tests.testAsyncGetPage(decoder, pages, 7);
      tests.testAsyncGetPage(decoder, pages, 8);
      tests.testAsyncGetPage(decoder, pages, 9);
      */

      tests.testAsyncGetPage(decoder, pages, 5, 22);
      tests.testAsyncGetPage(decoder, pages, 6, 23);
      tests.testAsyncGetPage(decoder, pages, 7, 25);
      tests.testAsyncGetPage(decoder, pages, 8, 22);
      tests.testAsyncGetPage(decoder, pages, 9, 24);


      // Anync page decoding.
      // tests.testAsyncGetPage(decoder, pages, 7, 15);
      //tests.testAsyncGetPage(decoder, pages, 5, 15);
    }
  );
});
