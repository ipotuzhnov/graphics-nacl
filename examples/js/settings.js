function loadSettings(type, path) {
  settings.setMimeType(type);
  settings.setDocumentPath(path);
}

var settings = (function() {
  var document = { path: null };
  var mimetype = null;
  
  function setDocumentPath(path) {
    settings.document.path = path;
  }
  
  function setMimeType(type) {
    if (type === 'ppapi') {
      settings.mimetype = 'application/x-ppapi';
    } else if (type === 'pnacl') {
      settings.mimetype = 'application/x-pnacl';
    }
  }
  
  // The symbols to export.
  return {
    // Properties
    document: document,
    mimetype: mimetype,

    // Functions
    setDocumentPath: setDocumentPath,
    setMimeType: setMimeType
  };
}());