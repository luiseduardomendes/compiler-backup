import os

def mudar_extensao_para_ll(diretorio):
    """
    Muda a extensão de arquivos sem extensão para .ll em um diretório específico.

    Args:
        diretorio (str): O caminho para o diretório onde os arquivos serão processados.
    """
    print(f"Verificando o diretório: {diretorio}")
    arquivos_renomeados = 0

    try:
        # Itera sobre todos os itens no diretório
        for nome_arquivo in os.listdir(diretorio):
            caminho_completo = os.path.join(diretorio, nome_arquivo)

            # Verifica se é um arquivo e se não possui extensão
            if os.path.isfile(caminho_completo) and '.' not in nome_arquivo:
                novo_caminho = f"{caminho_completo}.ll"
                try:
                    os.rename(caminho_completo, novo_caminho)
                    print(f"Renomeado: '{nome_arquivo}' para '{nome_arquivo}.ll'")
                    arquivos_renomeados += 1
                except OSError as e:
                    print(f"Erro ao renomear '{nome_arquivo}': {e}")

        print(f"\nProcesso concluído. Total de arquivos renomeados: {arquivos_renomeados}")

    except FileNotFoundError:
        print(f"Erro: O diretório '{diretorio}' não foi encontrado.")
    except Exception as e:
        print(f"Ocorreu um erro inesperado: {e}")

# --- Como usar ---
# Mude o caminho abaixo para o diretório que você deseja processar.
# Exemplos:
# pasta_alvo = "C:\\MinhaPastaComArquivos"  # Para Windows
# pasta_alvo = "/home/usuario/documentos/arquivos_sem_extensao" # Para Linux/macOS
# pasta_alvo = "." # Para a pasta atual onde o script está sendo executado

pasta_alvo = "." # Altere esta linha para o seu diretório alvo

if __name__ == "__main__":
    mudar_extensao_para_ll(pasta_alvo)