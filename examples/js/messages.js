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
  // Receiving page from JS
  // args: pp::Dictionary
  PPR_SEND_PAGE: "PPR_SEND_PAGE",

  // Request page pp::Dictionary
  PPD_GET_PAGE: "PPD_GET_PAGE",

  // Release page
  // args: ppVar.AsString() <pageId>
  PPD_RELEASE_PAGE: "PPD_RELEASE_PAGE",
  
  // Renderer reveived page
  // args = ppVar.AsString() <pageId>
  PPB_PAGE_RECEIVED: "PPB_PAGE_RECEIVED",
  // Render page
  PPR_PAGE_RENDER: "PPR_PAGE_RENDER",

  // Error
  // args: pp::Dictionary
  PPB_PLUGIN_ERROR: "PPB_PLUGIN_ERROR"
};
