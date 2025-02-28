#pragma once

namespace tpot
{
	struct ApplicationArgs
	{
		unsigned int 幅 = 1920;
		unsigned int 高さ = 1080;
		bool useWarpDevice = false;
	};

	class Application
	{
	public:
		Application(const ApplicationArgs& args);
		~Application();

		int Initialize(HWND hwnd);
		void Finalize();

		int Update();

	private:
		//  このアプリケーションでは、フレームバッファ数 の意味を一度に GPU にキューイングされる
		//  フレームの最大数とDXGI スワップチェインのバックバッファの数の両方を意味します。大半は、
		//  これは便利でうまく機能します。しかし、アプリケーションが利用可能なバックバッファ数よりも
		//  多くのフレームをキューに入れたい場合もあるでしょう。ユーザー入力に依存するフレームを過度に
		//  バッファリングすると、アプリケーションに顕著なレイテンシが発生しうることに
		//  注意する必要があります。
		static const UINT フレームバッファ数 = 2;

		// ビューポート
		UINT 幅_ = 1920;
		UINT 高さ_ = 1080;

		// アダプター情報
		// Windows Advanced Rasterization Platform(高速で完全に準拠したソフトウェアラスタライザー)の使用
		bool Warpデバイス使用_ = false;

		void ハードウェアアダプターの取得(
			_In_ IDXGIFactory1* pファクトリー,
			_Outptr_result_maybenull_ IDXGIAdapter1** ppアダプター,
			bool 高パフォーマンスアダプターの要求 = false);

		// パイプラインオブジェクト
		winrt::com_ptr<IDXGISwapChain3> スワップチェーン_ = nullptr;
		winrt::com_ptr<ID3D12Device> デバイス_ = nullptr;
		winrt::com_ptr<ID3D12CommandQueue> コマンドキュー_ = nullptr;
		winrt::com_ptr<ID3D12GraphicsCommandList> コマンドリスト_ = nullptr;
		winrt::com_ptr<ID3D12PipelineState> パイプラインステート_ = nullptr;
		winrt::com_ptr<ID3D12CommandAllocator> コマンドアロケーター_[フレームバッファ数] = {};
		winrt::com_ptr<ID3D12Resource> レンダーターゲット_[フレームバッファ数] = {};
		winrt::com_ptr<ID3D12DescriptorHeap> rtvヒープ_ = nullptr;
		UINT rtv記述子サイズ_ = 0;

		// 同期オブジェクト
		UINT バックバッファ番号_ = 0;
		HANDLE フェンスイベント_ = nullptr;
		winrt::com_ptr<ID3D12Fence> フェンス_ = nullptr;
		UINT64 フェンス値_[フレームバッファ数] = {};

		// Initialize 用
		void パイプラインの読み込み(HWND ウィンドウハンドル, unsigned int 幅, unsigned int 高さ);
		void アセットの読み込み();

		// 同期用
		void GPUを待つ();
		void 次のフレームに行く();

		// Update用
		void コマンドリストの記録();
	};
}