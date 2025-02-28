#include "framework.h" // Windowsの標準的なinclude
#include "Application.h"

namespace tpot
{
	Application::Application(const ApplicationArgs& 引数) : 
		幅_(引数.幅), 高さ_(引数.高さ), Warpデバイス使用_(引数.useWarpDevice)
    {
    }
	
	Application::~Application() 
	{
	}

    int Application::Initialize(HWND ウィンドウハンドル)
    {
        パイプラインの読み込み(ウィンドウハンドル, 幅_, 高さ_);
        アセットの読み込み();

        return 0;
    }

    void Application::Finalize()
    {
        // GPUが、デストラクタによってクリーンアップされようとしているリソースを
        // 参照しなくなっていることを確認
        GPUを待つ();

        CloseHandle(フェンスイベント_);
    }

    
    int Application::Update()
    {
        // シーンのレンダリングに必要なコマンドをすべてコマンドリストに記録
        コマンドリストの記録();

        // コマンドリストを実行
        ID3D12CommandList* pコマンドリスト列[] = { コマンドリスト_.get() };
        コマンドキュー_->ExecuteCommandLists(
            _countof(pコマンドリスト列), pコマンドリスト列);

        // フレームを(画面に)提示する
        winrt::check_hresult(スワップチェーン_->Present(1, 0));

        次のフレームに行く();

        return 0;
    }

    void Application::パイプラインの読み込み(HWND ウィンドウハンドル, 
        unsigned int 幅, unsigned int 高さ)
	{
        UINT dxgiファクトリーフラグ = 0;

#if defined(_DEBUG)
        // デバッグレイヤーを有効にする（グラフィックツールの「オプション機能」が必要）
        // 注意：デバイス作成後にデバッグ・レイヤーを有効にすると、アクティブ・デバイス
        // は無効になる
        {
            winrt::com_ptr<ID3D12Debug> デバッグコントローラー;
            if (SUCCEEDED(D3D12GetDebugInterface(
                __uuidof(デバッグコントローラー), デバッグコントローラー.put_void())))
            {
                デバッグコントローラー->EnableDebugLayer();

                // 追加のデバッグレイヤを有効化
                dxgiファクトリーフラグ |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }
#endif
        // DXGI オブジェクトを生成する
        winrt::com_ptr<IDXGIFactory4> ファクトリー;
        winrt::check_hresult(CreateDXGIFactory2(dxgiファクトリーフラグ, 
            __uuidof(ファクトリー), ファクトリー.put_void()));

		// デバイスを作成
        if (Warpデバイス使用_)
        {
            winrt::com_ptr<IDXGIAdapter> WARPアダプター;
            winrt::check_hresult(ファクトリー->EnumWarpAdapter(
                __uuidof(WARPアダプター), WARPアダプター.put_void()));

            winrt::check_hresult(D3D12CreateDevice(
                WARPアダプター.get(),
                D3D_FEATURE_LEVEL_12_0,// DX12の機能レベルに決め打ち
                __uuidof(デバイス_), デバイス_.put_void()
            ));
        }
        else
        {
            winrt::com_ptr<IDXGIAdapter1> ハードウェアアダプター;
            ハードウェアアダプターの取得(ファクトリー.get(), 
                ハードウェアアダプター.put());

            winrt::check_hresult(D3D12CreateDevice(
                ハードウェアアダプター.get(),
                D3D_FEATURE_LEVEL_12_0,// DX12の機能レベルに決め打ち
                __uuidof(デバイス_), デバイス_.put_void()
            ));
        }

        // コマンドキューについて記述し、作成する。
        D3D12_COMMAND_QUEUE_DESC キュー記述子 = {};
        キュー記述子.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        キュー記述子.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        winrt::check_hresult(デバイス_->CreateCommandQueue(&キュー記述子, 
            __uuidof(コマンドキュー_), コマンドキュー_.put_void()));

        // スワップチェーンを記述し、作成する。
        DXGI_SWAP_CHAIN_DESC1 スワップチェーン記述子 = {};
        スワップチェーン記述子.BufferCount = フレームバッファ数;
        スワップチェーン記述子.Width = 幅;
        スワップチェーン記述子.Height = 高さ;
        スワップチェーン記述子.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        スワップチェーン記述子.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        スワップチェーン記述子.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        スワップチェーン記述子.SampleDesc.Count = 1;

        winrt::com_ptr<IDXGISwapChain1> スワップチェーン;
        winrt::check_hresult(ファクトリー->CreateSwapChainForHwnd(
            コマンドキュー_.get(),// スワップチェーンは強制的なフラッシュにキューが必要
            ウィンドウハンドル,
            &スワップチェーン記述子,
            nullptr,
            nullptr,
            スワップチェーン.put()
        ));

        // このアプリケーションはフルスクリーンのトランジションをサポートしていない
        winrt::check_hresult(ファクトリー->MakeWindowAssociation(ウィンドウハンドル, 
            DXGI_MWA_NO_ALT_ENTER));

        スワップチェーン.as(スワップチェーン_);
        バックバッファ番号_ = スワップチェーン_->GetCurrentBackBufferIndex();

        // デスクリプターヒープを作成
        {
            // レンダー・ターゲット・ビュー（RTV）デスクリプターヒープを記述し、作成する。
            D3D12_DESCRIPTOR_HEAP_DESC rtvヒープ記述子 = {};
            rtvヒープ記述子.NumDescriptors = フレームバッファ数;
            rtvヒープ記述子.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvヒープ記述子.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            winrt::check_hresult(デバイス_->CreateDescriptorHeap(&rtvヒープ記述子, 
                __uuidof(rtvヒープ_), rtvヒープ_.put_void()));

            rtv記述子サイズ_ = デバイス_->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        // フレームリソースを作成
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvハンドル(
                rtvヒープ_->GetCPUDescriptorHandleForHeapStart());

            // フレームごとのRTVを作成
            for (UINT n = 0; n < フレームバッファ数; n++)
            {
                winrt::check_hresult(スワップチェーン_->GetBuffer(n, 
                    __uuidof(レンダーターゲット_[n]), レンダーターゲット_[n].put_void()));
                デバイス_->CreateRenderTargetView(レンダーターゲット_[n].get(), nullptr,
                    rtvハンドル);
                rtvハンドル.Offset(1, rtv記述子サイズ_);

                winrt::check_hresult(デバイス_->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(コマンドアロケーター_[n]), 
                    コマンドアロケーター_[n].put_void()));
            }
        }
    }

	void Application::アセットの読み込み()
	{
        // コマンドリストを生成
        winrt::check_hresult(デバイス_->CreateCommandList(0, 
            D3D12_COMMAND_LIST_TYPE_DIRECT, 
            コマンドアロケーター_[バックバッファ番号_].get(), 
            パイプラインステート_.get(), 
            __uuidof(コマンドリスト_), コマンドリスト_.put_void()));

        // コマンドリストは記録状態で作成されるが、まだ記録するものは何もない。
        // メイン・ループではリストがクローズされていることが期待されている。
        winrt::check_hresult(コマンドリスト_->Close());


        // 同期オブジェクトを作成し、アセットがGPUにアップロードされるまで待つ。
        {
            winrt::check_hresult(デバイス_->CreateFence(
                フェンス値_[バックバッファ番号_], D3D12_FENCE_FLAG_NONE, 
                __uuidof(フェンス_), フェンス_.put_void()));
            フェンス値_[バックバッファ番号_]++;

            // フレーム同期に使用するイベントハンドルを作成する
            フェンスイベント_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (フェンスイベント_ == nullptr)
            {
                winrt::check_hresult(HRESULT_FROM_WIN32(GetLastError()));
            }

            // コマンドリストの実行を待つ。メインループで同じコマンドリストを再利用して
            // おり、今はセットアップが完了するのを待ってから続けたい。
            GPUを待つ();
        }
    }


    void Application::コマンドリストの記録()
    {
       // コマンド・リスト・アロケータは、関連するコマンド・リストがGPUでの
       // 実行を終了したときにのみリセットすることができる。
       // アプリはGPU実行の進捗の判断にフェンスを使うべき。
        winrt::check_hresult(コマンドアロケーター_[バックバッファ番号_]->Reset());

        // コマンド・リストでExecuteCommandList()を呼び出すと、
        // そのコマンド・リストはいつでもリセットすることができるが、
        // 再記録の前にリセットしなければならない。
        winrt::check_hresult(コマンドリスト_->Reset(
            コマンドアロケーター_[バックバッファ番号_].get(), 
            パイプラインステート_.get()));

        // バックバッファをレンダーターゲットとして使用する
		ID3D12Resource* レンダーターゲット = レンダーターゲット_[バックバッファ番号_].get();
        CD3DX12_RESOURCE_BARRIER バリアPresent2RT(CD3DX12_RESOURCE_BARRIER::Transition(
            レンダーターゲット,
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
        コマンドリスト_->ResourceBarrier(1, &バリアPresent2RT);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvハンドル(
            rtvヒープ_->GetCPUDescriptorHandleForHeapStart(), 
            バックバッファ番号_, rtv記述子サイズ_);
        コマンドリスト_->OMSetRenderTargets(1, &rtvハンドル, FALSE, nullptr);

        // コマンドを記録
		const float 背景色[] = { 0.0f, 0.2f, 0.4f, 1.0f };// 赤、緑、青、アルファ
        コマンドリスト_->ClearRenderTargetView(rtvハンドル, 背景色, 0, nullptr);

        // バックバッファは画面更新に使用される
		CD3DX12_RESOURCE_BARRIER バリアRT2Present(CD3DX12_RESOURCE_BARRIER::Transition(
            レンダーターゲット,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
        コマンドリスト_->ResourceBarrier(1, &バリアRT2Present);

        winrt::check_hresult(コマンドリスト_->Close());
    }

    void Application::GPUを待つ()
    {
        // キューにシグナルコマンドをスケジュールする
        winrt::check_hresult(コマンドキュー_->Signal(フェンス_.get(), 
            フェンス値_[バックバッファ番号_]));

        // フェンスの処理が終わるまで待つ
        winrt::check_hresult(フェンス_->SetEventOnCompletion(
            フェンス値_[バックバッファ番号_], フェンスイベント_));
        WaitForSingleObjectEx(フェンスイベント_, INFINITE, FALSE);

        // 現在のフレームのフェンス値を増加させる
        フェンス値_[バックバッファ番号_]++;
    }

    // 次のフレームのレンダリングを準備
    void Application::次のフレームに行く()
    {
        // キューにシグナルコマンドをスケジュールする
        const UINT64 現在のフェンス値 = フェンス値_[バックバッファ番号_];
        winrt::check_hresult(コマンドキュー_->Signal(フェンス_.get(), 現在のフェンス値));

        // フレームインデックスを更新
        バックバッファ番号_ = スワップチェーン_->GetCurrentBackBufferIndex();

        // 次のフレームをレンダリングする準備がまだできていない場合は、
        // 準備ができるまで待つ。
        if (フェンス_->GetCompletedValue() < フェンス値_[バックバッファ番号_])
        {
            winrt::check_hresult(フェンス_->SetEventOnCompletion(
                フェンス値_[バックバッファ番号_], フェンスイベント_));
            WaitForSingleObjectEx(フェンスイベント_, INFINITE, FALSE);
        }

        // 次のフレームのフェンス値を設定する
        フェンス値_[バックバッファ番号_] = 現在のフェンス値 + 1;
    }

    void Application::ハードウェアアダプターの取得(
        IDXGIFactory1* pファクトリー,
        IDXGIAdapter1** ppアダプター,
        bool 高パフォーマンスアダプターの要求)
    {
        *ppアダプター = nullptr;

        winrt::com_ptr<IDXGIAdapter1> アダプター;

        winrt::com_ptr<IDXGIFactory6> ファクトリー6;
        if (winrt::check_hresult(pファクトリー->QueryInterface(
            __uuidof(ファクトリー6), ファクトリー6.put_void())))
        {
            for (
                UINT アダプター番号 = 0;
                winrt::check_hresult(ファクトリー6->EnumAdapterByGpuPreference(
                    アダプター番号,
                    高パフォーマンスアダプターの要求 == true ? 
                        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : 
                        DXGI_GPU_PREFERENCE_UNSPECIFIED,
                    __uuidof(アダプター), アダプター.put_void()));
                    ++アダプター番号)
            {
                DXGI_ADAPTER_DESC1 記述子;
                アダプター->GetDesc1(&記述子);

                if (記述子.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Basic Render Driverアダプタは選択しないでください。
                    // ソフトウェア・アダプタが必要な場合は、コマンドラインに「/warp 」
                    // と入力してください。
                    continue;
                }

                // アダプタがDirect3D 12をサポートしているかどうかを確認するが、
                // 実際のデバイスはまだ作成しない
                if (winrt::check_hresult(D3D12CreateDevice(アダプター.get(),
                    D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }
        }

        if (アダプター.get() == nullptr)
        {
            for (UINT アダプター番号 = 0; winrt::check_hresult(
                pファクトリー->EnumAdapters1(アダプター番号, アダプター.put()));
                ++アダプター番号)
            {
                DXGI_ADAPTER_DESC1 記述子;
                アダプター->GetDesc1(&記述子);

                if (記述子.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Basic Render Driverアダプタは選択しないでください。
                    // ソフトウェア・アダプタが必要な場合は、コマンドラインに「/warp 」
                    // と入力してください。
                    continue;
                }

                // アダプタがDirect3D 12をサポートしているかどうかを確認するが、
                // 実際のデバイスはまだ作成しない
                if (winrt::check_hresult(D3D12CreateDevice(アダプター.get(),
                    D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }
        }

        *ppアダプター = アダプター.detach();
    }
}
