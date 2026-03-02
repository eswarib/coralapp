const fs = require('fs');
const path = require('path');
const os = require('os');
const { ipcRenderer } = require('electron');

function getAppImageMountPath() {
  if (process.env.APPIMAGE) {
    return path.join(path.dirname(process.execPath), '..', '..', '..');
  }
  return null;
}

// Resolve config.json path depending on environment (AppImage vs dev)
let configPath;
if (process.env.APPIMAGE) {
  const appImageMountPath = getAppImageMountPath();
  const defaultConfigPath = path.join(appImageMountPath, 'usr', 'share', 'coral', 'conf', 'config.json');
  const userConfigDir = path.join(os.homedir(), '.coral');
  const userConfigPath = path.join(userConfigDir, 'conf', 'config.json');
  try {
    if (!fs.existsSync(userConfigDir)) {
      fs.mkdirSync(userConfigDir, { recursive: true });
    }
    if (!fs.existsSync(userConfigPath) && fs.existsSync(defaultConfigPath)) {
      fs.copyFileSync(defaultConfigPath, userConfigPath);
    }
  } catch (e) {
    console.error('Failed to prepare user config:', e.message);
  }
  configPath = userConfigPath;
} else {
  // Development: always use ~/.coral/conf/config.json; copy from platform config on first run
  const userConfigDir = path.join(os.homedir(), '.coral');
  configPath = path.join(userConfigDir, 'conf', 'config.json');
  const platformConfig = process.platform === 'win32' ? 'config.json' : 'config-linux.json';
  const devDefault = path.join(__dirname, '../..', 'coral', 'conf', platformConfig);
  try {
    if (!fs.existsSync(path.dirname(configPath))) fs.mkdirSync(path.dirname(configPath), { recursive: true });
    if (!fs.existsSync(configPath) && fs.existsSync(devDefault)) {
      fs.copyFileSync(devDefault, configPath);
    }
  } catch (e) { console.error('Failed to prepare user config:', e.message); }
}
const configForm = document.getElementById('configForm');
const saveBtn = document.getElementById('saveBtn');
const defaultBtn = document.getElementById('defaultBtn');
const statusDiv = document.getElementById('status');

loadConfig();

let config = {};

function formatKeyComboFromEvent(e) {
  const parts = [];
  if (e.ctrlKey) parts.push('ctrl');
  if (e.altKey) parts.push('alt');
  if (e.shiftKey) parts.push('shift');
  if (e.metaKey) parts.push('meta');

  let key = '';
  if (e.code && e.code.startsWith('Key')) {
    key = e.code.slice(3).toLowerCase();
  } else if (e.code && e.code.startsWith('Digit')) {
    key = e.code.slice(5);
  } else if (e.key && e.key.length === 1) {
    key = e.key.toLowerCase();
  } else if (e.key) {
    key = e.key.toLowerCase(); // e.g., 'enter', 'escape', 'space'
  }
  // Avoid pure modifier shortcuts
  const isOnlyModifier = !key || ['control','ctrl','alt','shift','meta','super'].includes(key);
  if (!isOnlyModifier) parts.push(key);
  return parts.join('+');
}

function createField(labelText, inputEl) {
  const row = document.createElement('div');
  row.className = 'field';
  const lbl = document.createElement('div');
  lbl.className = 'field-label';
  lbl.textContent = labelText;
  const wrap = document.createElement('div');
  wrap.className = 'field-input';
  wrap.appendChild(inputEl);
  row.appendChild(lbl);
  row.appendChild(wrap);
  return row;
}

function renderMainForm(cfg) {
  configForm.innerHTML = '';
  const primaryKeys = ['triggerKey', 'cmdTriggerKey', 'triggerMode'];
  const modelKey = 'whisperModelPath';

  // --- Trigger settings card ---
  const triggerLabel = document.createElement('div');
  triggerLabel.className = 'section-label';
  triggerLabel.textContent = 'Trigger';
  configForm.appendChild(triggerLabel);

  const triggerCard = document.createElement('div');
  triggerCard.className = 'card';

  // triggerKey
  const triggerInput = document.createElement('input');
  triggerInput.type = 'text';
  triggerInput.readOnly = true;
  triggerInput.placeholder = 'Click, then press shortcut';
  triggerInput.value = cfg.triggerKey || '';
  triggerInput.name = 'triggerKey';
  triggerInput.addEventListener('keydown', (e) => {
    e.preventDefault();
    e.stopPropagation();
    const combo = formatKeyComboFromEvent(e);
    if (combo) triggerInput.value = combo;
  });
  triggerInput.addEventListener('focus', () => { statusDiv.textContent = 'Press desired shortcut…'; statusDiv.className = ''; });
  triggerInput.addEventListener('blur', () => { statusDiv.textContent = ''; });
  triggerCard.appendChild(createField('Trigger key', triggerInput));

  // cmdTriggerKey
  const cmdInput = document.createElement('input');
  cmdInput.type = 'text';
  cmdInput.readOnly = true;
  cmdInput.placeholder = 'Click, then press shortcut';
  cmdInput.value = cfg.cmdTriggerKey || '';
  cmdInput.name = 'cmdTriggerKey';
  cmdInput.addEventListener('keydown', (e) => {
    e.preventDefault();
    e.stopPropagation();
    const combo = formatKeyComboFromEvent(e);
    if (combo) cmdInput.value = combo;
  });
  cmdInput.addEventListener('focus', () => { statusDiv.textContent = 'Press desired shortcut…'; statusDiv.className = ''; });
  cmdInput.addEventListener('blur', () => { statusDiv.textContent = ''; });
  triggerCard.appendChild(createField('Cmd key', cmdInput));

  // triggerMode
  const modeSelect = document.createElement('select');
  modeSelect.name = 'triggerMode';
  [['pushToTalk', 'Push-to-talk (hold)'], ['continuous', 'Continuous (double-tap)']].forEach(([val, txt]) => {
    const opt = document.createElement('option');
    opt.value = val;
    opt.textContent = txt;
    if ((cfg.triggerMode || 'pushToTalk') === val) opt.selected = true;
    modeSelect.appendChild(opt);
  });
  triggerCard.appendChild(createField('Mode', modeSelect));

  configForm.appendChild(triggerCard);

  // --- Model card ---
  const modelLabel = document.createElement('div');
  modelLabel.className = 'section-label';
  modelLabel.textContent = 'Model';
  configForm.appendChild(modelLabel);

  const modelCard = document.createElement('div');
  modelCard.className = 'card';

  const hidden = document.createElement('input');
  hidden.type = 'hidden';
  hidden.name = modelKey;
  hidden.value = cfg[modelKey] || '';

  const displayForPath = (p) => {
    const v = (p || '').toLowerCase();
    if (v.includes('ggml-base.en.bin')) return 'Whisper base';
    if (v.includes('ggml-small.en-q8_0.bin') || v.includes('ggml-small.en.bin')) return 'Whisper small';
    if (!v) return 'Whisper base';
    return 'Custom';
  };

  const modelWrap = document.createElement('div');
  modelWrap.className = 'model-selector';
  const pill = document.createElement('span');
  pill.className = 'pill-select';
  pill.textContent = displayForPath(cfg[modelKey]);
  const menu = document.createElement('div');
  menu.className = 'dropdown-menu';
  menu.style.display = 'none';

  ['Whisper base', 'Whisper small', 'Custom'].forEach(txt => {
    const it = document.createElement('div');
    it.className = 'dropdown-item';
    it.textContent = txt;
    it.onclick = async () => {
      pill.textContent = txt;
      menu.style.display = 'none';
      if (txt === 'Whisper base') hidden.value = 'ggml-base.en.bin';
      else if (txt === 'Whisper small') hidden.value = 'ggml-small.en-q8_0.bin';
      else {
        const fp = await ipcRenderer.invoke('select-model-file');
        if (fp) hidden.value = fp;
      }
      requestResize();
    };
    menu.appendChild(it);
  });

  pill.onclick = () => {
    menu.style.display = (menu.style.display === 'none') ? 'block' : 'none';
    requestResize();
  };
  document.addEventListener('click', (e) => {
    if (!modelWrap.contains(e.target)) menu.style.display = 'none';
  });

  modelWrap.appendChild(pill);
  modelWrap.appendChild(menu);
  modelWrap.appendChild(hidden);

  const modelRow = document.createElement('div');
  modelRow.className = 'field';
  const modelLbl = document.createElement('div');
  modelLbl.className = 'field-label';
  modelLbl.textContent = 'Whisper model';
  const modelInputWrap = document.createElement('div');
  modelInputWrap.className = 'field-input';
  modelInputWrap.appendChild(modelWrap);
  modelRow.appendChild(modelLbl);
  modelRow.appendChild(modelInputWrap);
  modelCard.appendChild(modelRow);

  configForm.appendChild(modelCard);

  requestResize();
}

function requestResize() {
  try {
    const formBottom = configForm.getBoundingClientRect().bottom;
    const desired = Math.ceil(window.scrollY + formBottom + 24);
    ipcRenderer.send('resize-window', desired);
    // Recheck shortly after layout/paint to catch late style recalcs
    setTimeout(() => {
      const formBottom2 = configForm.getBoundingClientRect().bottom;
      const desired2 = Math.ceil(window.scrollY + formBottom2 + 24);
      ipcRenderer.send('resize-window', desired2);
    }, 50);
  } catch (_) {}
}

window.addEventListener('load', () => {
  requestResize();
});

function loadConfig() {
  fs.readFile(configPath, 'utf8', (err, data) => {
    if (err) {
      statusDiv.textContent = 'Failed to load config: ' + err.message;
      statusDiv.className = 'error';
      return;
    }
    try {
      config = JSON.parse(data);
      renderMainForm(config);
      statusDiv.textContent = '';
      statusDiv.className = '';
    } catch (e) {
      statusDiv.textContent = 'Invalid config.json: ' + e.message;
      statusDiv.className = 'error';
    }
  });
}

saveBtn.onclick = (e) => {
  e.preventDefault();
  const newConfig = { ...config };
  const booleanSelects = new Set();
  for (const el of configForm.elements) {
    if (!el.name) continue;
    if (el.tagName === 'SELECT') {
      if (el.querySelector('option[value="true"]') && el.querySelector('option[value="false"]')) {
        newConfig[el.name] = el.value === 'true';
      } else {
        newConfig[el.name] = el.value;
      }
    } else if (el.type === 'number') {
      newConfig[el.name] = Number(el.value);
    } else {
      newConfig[el.name] = el.value;
    }
  }
  // Validate model selection: allow known tokens or a valid file
  try {
    const modelPath = newConfig['whisperModelPath'];
    const tokens = ['ggml-base.en.bin', 'ggml-small.en.bin', 'ggml-small.en-q8_0.bin', ''];
    const isToken = tokens.includes(modelPath);
    if (!isToken) {
      if (!fs.existsSync(modelPath) || !fs.statSync(modelPath).isFile()) {
        statusDiv.textContent = 'Please select a valid model file.';
        statusDiv.className = 'error';
        return;
      }
    }
  } catch (_) {}
  fs.writeFile(configPath, JSON.stringify(newConfig, null, 2), (err) => {
    if (err) {
      statusDiv.textContent = 'Failed to save: ' + err.message;
      statusDiv.className = 'error';
    } else {
      statusDiv.textContent = 'Settings saved!';
      statusDiv.className = 'success';
      try {
        window?.require && ipcRenderer.send('config-updated');
      } catch (_) {}
      setTimeout(() => { try { window.close(); } catch (_) {} }, 400);
    }
  });
};

defaultBtn.onclick = (e) => {
  e.preventDefault();
  try {
    let defaultConfigPath;
    if (process.env.APPIMAGE) {
      const appImageMountPath = getAppImageMountPath();
      defaultConfigPath = path.join(appImageMountPath, 'usr', 'share', 'coral', 'conf', 'config.json');
    } else {
      const platformConfig = process.platform === 'win32' ? 'config.json' : 'config-linux.json';
      defaultConfigPath = path.join(__dirname, '../..', 'coral', 'conf', platformConfig);
    }
    const data = fs.readFileSync(defaultConfigPath, 'utf8');
    config = JSON.parse(data);
    // Write defaults into user config and re-render form
    fs.writeFile(configPath, JSON.stringify(config, null, 2), (err) => {
      if (err) {
        statusDiv.textContent = 'Failed to set defaults: ' + err.message;
      } else {
        renderMainForm(config);
        statusDiv.textContent = 'Defaults applied';
        try { ipcRenderer.send('config-updated'); } catch (_) {}
      }
    });
  } catch (ex) {
    statusDiv.textContent = 'Failed to load defaults: ' + (ex && ex.message ? ex.message : ex);
  }
};
