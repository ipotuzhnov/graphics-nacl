var messages = {
  // Messages
  // ppb - Pepper Browser
  // ppp - Pepper Plugin
  // ppd - Graphics NaCl Decoder
  // ppr - Graphics NaCl Renderer

  // Download document
  PPD_DOWNLOAD_START: "PPD_DOWNLOAD_START",
  // Update downloading progress
  // args: pp::Var.AsInt()
  PPB_DOWNLOAD_PROGRESS: "PPB_DOWNLOAD_PROGRESS",
  // Notify browser that download is finished
  PPB_DOWNLOAD_FINISHED: "PPB_DOWNLOAD_FINISHED",

  // Start decoding document
  PPD_DECODE_START: "PPD_DECODE_START",
  // Notify browser that decode is finished
  // args = pp::VarArray <pp::VarDictionary<width, height, dpi>>
  PPB_DECODE_FINISHED: "PPB_DECODE_FINISHED",
  // Decode page
  // args = pp::Dictionary <pp::VarDictionary<pageId, pageNum, width, height>>
  PPD_DECODE_PAGE: "PPD_DECODE_PAGE",
  
  // Page is decoded
  // args: pageId
  PPR_PAGE_READY: "PPR_PAGE_READY",
  // Request page
  // Request page pp::Dictionary
  PPD_GET_PAGE: "PPD_GET_PAGE",
  // Receiving page from JS
  // args: pp::Dictionary
  PPR_SEND_PAGE: "PPR_SEND_PAGE",
  
  // Request page as base64
  // args = pp::Var.asString() <pageId>
  PPD_GET_PAGE_AS_BASE64: "PPD_GET_PAGE_AS_BASE64",
  // Send page to browser as base64
  // args = pp::Dictionary
  PPB_SEND_PAGE_AS_BASE64: "PPB_SEND_PAGE_AS_BASE64",

  // Release page
  // args: pp::Var.AsString() <pageId>
  PPD_RELEASE_PAGE: "PPD_RELEASE_PAGE",
  
  // Renderer received page
  // args = pp::Var.AsString() <pageId>
  PPB_PAGE_RECEIVED: "PPB_PAGE_RECEIVED",
  // Render page
  PPR_PAGE_RENDER: "PPR_PAGE_RENDER",

  // Error
  // args: pp::Dictionary
  PPB_PLUGIN_ERROR: "PPB_PLUGIN_ERROR"
};
