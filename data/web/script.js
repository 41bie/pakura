const listElement = document.querySelector("#network-list");
const previousButton = document.querySelector("#previous-page");
const nextButton = document.querySelector("#next-page");
const pageStatus = document.querySelector("#page-status");
const detailModal = document.querySelector("#detail-modal");
const detailTitle = document.querySelector("#detail-title");
const detailFields = document.querySelector("#detail-fields");
const closeDetailButton = document.querySelector("#close-detail");

let currentPage = 0;
let pageCount = 0;

function makeMaskedName(prefix) {
	const name = document.createElement("span");
	name.className = "network-name";
	name.textContent = prefix;
	const mask = document.createElement("span");
	mask.className = "ssid-mask";
	mask.setAttribute("aria-label", "SSID hidden");
	name.append(mask);
	return name;
}

function makeButton(label, className, handler) {
	const button = document.createElement("button");
	button.type = "button";
	button.className = className;
	button.textContent = label;
	button.addEventListener("click", handler);
	return button;
}

async function showDetail(id) {
	const response = await fetch(`/api/ssid?id=${encodeURIComponent(id)}`);
	if (!response.ok) {
		return;
	}
	const record = await response.json();
	detailTitle.textContent = `NETWORK ${record.id}`;
	detailFields.replaceChildren();
	for (const [label, value] of [
		["SSID", record.ssid],
		["BSSID", record.bssid],
		["RSSI", `${record.rssi} dBm`],
		["CHANNEL", record.channel],
		["SECURITY", record.security],
	]) {
		const term = document.createElement("dt");
		term.textContent = label;
		const description = document.createElement("dd");
		description.textContent = value;
		detailFields.append(term, description);
	}
	detailModal.hidden = false;
}

async function revealSSID(id, nameElement) {
	const response = await fetch(`/api/ssid?id=${encodeURIComponent(id)}`);
	if (!response.ok) {
		return;
	}
	const record = await response.json();
	nameElement.textContent = record.ssid;
}

async function loadNetworks(page) {
	const response = await fetch(`/api/ssids?page=${page}`);
	if (!response.ok) {
		listElement.textContent = "UNABLE TO LOAD NETWORKS";
		return;
	}
	const data = await response.json();
	currentPage = data.page;
	pageCount = data.pageCount;
	if (data.records.length > 0) {
		const newestId = data.records[0].id;
		const oldestId = data.records[data.records.length - 1].id;
		pageStatus.textContent = `SSIDs ${newestId}-${oldestId}`;
	}
	else {
		pageStatus.textContent = "SSIDs 0";
	}
	previousButton.disabled = currentPage <= 0 || pageCount === 0;
	nextButton.disabled = currentPage >= pageCount - 1 || pageCount === 0;
	listElement.replaceChildren();
	for (const record of data.records) {
		const row = document.createElement("article");
		row.className = "network-row";
		const nameElement = makeMaskedName(record.prefix);
		row.append(
			nameElement,
			makeButton("VIEW", "view-button", () => revealSSID(record.id, nameElement)),
			makeButton("DETAILS", "detail-button", () => showDetail(record.id)),
		);
		listElement.append(row);
	}
}

previousButton.addEventListener("click", () => loadNetworks(currentPage - 1));
nextButton.addEventListener("click", () => loadNetworks(currentPage + 1));
closeDetailButton.addEventListener("click", () => {
	detailModal.hidden = true;
});
detailModal.addEventListener("click", (event) => {
	if (event.target === detailModal) {
		detailModal.hidden = true;
	}
});

loadNetworks(0);

