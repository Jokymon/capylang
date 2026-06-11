import { ExtensionContext, Uri, window, workspace, commands } from 'vscode';
import { LanguageClient, LanguageClientOptions, ServerOptions, RequestType } from 'vscode-languageclient/node';
import { Wasm, ProcessOptions } from '@vscode/wasm-wasi/v1';
import { createStdioOptions, createUriConverters, startServer } from '@vscode/wasm-wasi-lsp';

let client: LanguageClient;

export async function activate(context: ExtensionContext) {
	const channel = window.createOutputChannel('Capylang LSP WASM Server');
	channel.appendLine('Activating Capylang extension');

	let wasm: Wasm;
	try {
		wasm = await Wasm.load();
		channel.appendLine('WASM runtime loaded');
	} catch (error) {
		channel.appendLine(`Failed to load WASM runtime: ${String(error)}`);
		throw error;
	}

	const serverOptions: ServerOptions = async () => {
		const options: ProcessOptions = {
			stdio: createStdioOptions(),
			mountPoints: [
				{ kind: 'workspaceFolder' },
			]
		};
		const filename = Uri.joinPath(context.extensionUri, 'server', 'build', 'capylsp.wasm');
		channel.appendLine(`Loading server module from ${filename.toString()}`);
		const bits = await workspace.fs.readFile(filename) as Uint8Array<ArrayBuffer>;
		const module = await WebAssembly.compile(bits);
		const process = await wasm.createProcess('lsp-server', module, { initial: 160, maximum: 160, shared: true }, options);

		const decoder = new TextDecoder('utf-8');
		process.stderr!.onData((data) => {
			channel.append(decoder.decode(data));
		});

		return startServer(process);
	};

	const clientOptions: LanguageClientOptions = {
		documentSelector: [{ language: 'capy' }],
		// @ts-ignore
		outputChannel: channel,
		uriConverters: createUriConverters(),
	};

	client = new LanguageClient('lspClient', 'LSP Client', serverOptions, clientOptions);
	try {
		await client.start();
		channel.appendLine('Language client started');
	} catch (error) {
		channel.appendLine(`Language client start failed: ${String(error)}`);
		client.error(`Start failed`, error, 'force');
		throw error;
	}
}

export function deactivate() {
	return client?.stop();
}
