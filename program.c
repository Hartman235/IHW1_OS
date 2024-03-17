#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

bool isVowel(char c) {
    c = toupper(c);
    return (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U');
}


void replaceVowelsWithHex(char input[], char output[]) {
    for (int i = 0; i < strlen(input); i++) {
        if (isVowel(input[i])) {
            sprintf(output + strlen(output), "{0x%X}", input[i]); // Заменяем гласные на их ASCII коды в шестнадцатеричной форме
        } else {
            sprintf(output + strlen(output), "%c", input[i]); // Остальные символы оставляем без изменений
        }
    }
}

int main() {
    char input[100];
    char output[1000] = ""; // Пусть выходная строка может быть достаточно большой для хранения результата
    printf("Input string: ");
    fgets(input, sizeof(input), stdin);

    replaceVowelsWithHex(input, output);

    printf("New string: %s\n", output);

    return 0;
}
