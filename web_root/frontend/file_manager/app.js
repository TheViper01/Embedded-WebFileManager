// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved

var getFilePath = function(){
  var uploadPath = "";
  var pathArray = window.location.pathname.split('/');
  
  for (let i = 2; i < pathArray.length-1; i++) {
    uploadPath += '/';
    uploadPath += pathArray[i];
    
  }
  return uploadPath;
}

var getPrevDirPath = function(){
  var uploadPath = "";
  var pathArray = getFilePath().split('/');
  
  for (let i = 1; i < pathArray.length-1; i++) {
    uploadPath += '/';
    uploadPath += pathArray[i];
    
  }
  if(uploadPath.length == 0){
    uploadPath = "/";
  }
  else if(uploadPath[-1] != '/'){
    uploadPath += '/';
  }
  return uploadPath;
}

var getJSON = function(url, callback) {
  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);
  xhr.responseType = 'json';
  xhr.onload = function() {
    var status = xhr.status;
    if (status === 200) {
      callback(null, xhr.response);
    } else {
      callback(status, xhr.response);
    }
  };
  xhr.send();
};

var postJSON = function(url, callback) {
  var xhr = new XMLHttpRequest();
  xhr.open('POST', url, true);
  xhr.responseType = 'json';
  xhr.onload = function() {
    var status = xhr.status;
    if (status === 200) {
      callback(null, xhr.response);
    } else {
      callback(status, xhr.response);
    }
  };
  xhr.send();
};

var deleteJSON = function(url, callback) {
  var xhr = new XMLHttpRequest();
  xhr.open('DELETE', url, true);
  xhr.responseType = 'json';
  xhr.onload = function() {
    var status = xhr.status;
    if (status === 200) {
      callback(null, xhr.response);
    } else {
      callback(status, xhr.response);
    }
  };
  xhr.send();
};


var deleteFile = function(filename){
  deleteJSON(window.location.protocol + "//" + window.location.host + '/api/files' + getFilePath() + "/" + filename,
function(err, data) {
  if (err !== null) {
    console.log('Something went wrong: ' + err);
    //alert('Something went wrong: ' + err);
  } else {
    console.log("deleted: " + JSON.stringify(data));
    //alert('data: ' + data);
  }
  location.reload();
});
};

var createDirectory = function(dirname){
  postJSON(window.location.protocol + "//" + window.location.host + '/api/files' + getFilePath() + "/" + dirname + "/",
  function(err, data) {
    if (err !== null) {
      console.log('Something went wrong: ' + err);
      //alert('Something went wrong: ' + err);
    } else {
      console.log("dir created: " + JSON.stringify(data));
      //alert('data: ' + data);
    }
    location.reload();
  });
}

var createDirectorySubmit = function(){
  let dirname = document.getElementById('newdirname').value;
  createDirectory(encodeURIComponent(dirname));
}


var createFileTable = function(files_, directoris_, tbodyId){
  let headerNColumns = 7;
  const tbody = document.getElementById(tbodyId);
  let col = [];
  let file = null;
  let directory = null;
  let tabCell = null;


  tbody.innerHTML = '';

  tr = tbody.insertRow(-1);
  tabCell = tr.insertCell(-1);
  tabCell.innerHTML =  '<a href="' + '/resources' + getPrevDirPath() + '">..</a>';

  for(let i=0; i < directoris_.length; i++){
    directory = directoris_[i];
    tr = tbody.insertRow(-1);

    tabCell = tr.insertCell(-1);
    tabCell.innerHTML =  '<a href="' + '/resources' + getFilePath() + "/" +  encodeURIComponent(directory.name) + '/">' + directory.name + '</a>';

    tabCell = tr.insertCell(-1);
    tabCell.innerHTML = directory.modified;

    tabCell = tr.insertCell(-1);

    tabCell = tr.insertCell(-1);
    tabCell.innerHTML =  `<a href=javascript:deleteFile("${encodeURIComponent(directory.name)}/")>delete</a>`;
  }



  for(let i=0; i < files_.length; i++){
    file = files_[i];
    tr = tbody.insertRow(-1);

    tabCell = tr.insertCell(-1);
    tabCell.innerHTML = file.name;

    tabCell = tr.insertCell(-1);
    tabCell.innerHTML = file.modified;

    tabCell = tr.insertCell(-1);
    tabCell.innerHTML = file.size;

    tabCell = tr.insertCell(-1);
    tabCell.innerHTML =  `<a href=javascript:deleteFile("${encodeURIComponent(file.name)}")>delete</a>`;

    tabCell = tr.insertCell(-1);
    tabCell.innerHTML =  `<a href="/api/files${getFilePath()}/${encodeURIComponent(file.name)}" download="">download</a>`;
    
    console.log(file.name.replaceAll(" ", "-"));
  }
};


// Helper function to display upload status
var setStatus = function(text) {
  document.getElementById('el3').innerText = text;
};

// When user clicks on a button, trigger file selection dialog
var button = document.getElementById('el2');
button.onclick = function(ev) {
  input.click();
};



// Send a large blob of data chunk by chunk
var sendFileData = function(name, data, chunkSize) {
  var sendChunk = function(offset) {
    var chunk = data.subarray(offset, offset + chunkSize) || '';
    var opts = {method: 'PUT', body: chunk};
    var url = '/api/files' + getFilePath() + '/' + encodeURIComponent(name) + '?offset=' + offset;
    setStatus(
        'sending bytes ' + offset + '..' + (offset + chunk.length) + ' of ' +
        data.length);
    fetch(url, opts).then(function(res) {
      if (chunk.length > 0) sendChunk(offset + chunk.length);
    });
  };
  sendChunk(0);
};

// If user selected a file, read it into memory and trigger sendFileData()
var input = document.getElementById('el1');
input.onchange = function(ev) {
  if (!ev.target.files[0]) return;
  var f = ev.target.files[0], r = new FileReader();
  r.readAsArrayBuffer(f);
  r.onload = function() {
    ev.target.value = '';
    sendFileData(f.name, new Uint8Array(r.result), 4096);
  };
};


/*===============================================================================================================================*/

var listDirContent = null;

var files = null;
var directories = null;

getJSON(window.location.protocol + "//" + window.location.host + '/api/files' + getFilePath(),
function(err, data) {
  if (err !== null) {
    console.log('Something went wrong: ' + err);
    //alert('Something went wrong: ' + err);
  } else {
    console.log("data: " + JSON.stringify(data));
    listDirContent = data;
    files = data.file;
    directories = data.dir;
    createFileTable(files, directories, "tbody_");
    //alert('data: ' + data);
  }
});





